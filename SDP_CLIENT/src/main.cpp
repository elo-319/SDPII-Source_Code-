#include "Arduino.h"
#include "SPI.h"
#include "heartbeat.h"
#include <Wire.h>
#include "LoRa.h"
#include <SparkFun_u-blox_GNSS_Arduino_Library.h>
#include "CRC16.h"

struct packet
{
  char data[192];
  uint16_t crc;
} myPacket;

CRC16 crc;

long latitude, longitude, altitude;
uint8_t SIV;
const uint32_t devid = 89316UL;

void initGPS();
void initLoRa();
void initHB();
void getGPSData();
void sendLoRa();
#define GPS_BAND_RATE 9600
#define LORA_BAND 868E6

#define LORA_MISO 19
#define LORA_CS 18
#define LORA_MOSI 27
#define LORA_SCK 5
#define LORA_RST 14
#define LORA_IRQ 26

SFE_UBLOX_GNSS myGPS;
#define GPS_RX_PIN 34
#define GPS_TX_PIN 12

uint8_t contactsts, bpm;
uint32_t gps_period = 10000UL, gps_ticks;
uint32_t lora_period = 5000UL, lora_ticks;

bool sendonce = true;
void setup()
{

  Serial.begin(115200);
  initGPS();
  initLoRa();
  initHB();

  randomSeed(analogRead(A0));
}

void loop()
{

  heartbeat_main(&bpm, &contactsts);

  if (millis() - gps_ticks >= gps_period)
  {

    getGPSData();
    gps_ticks = millis();
    lora_ticks = gps_ticks;
  }
  if (millis() - lora_ticks >= lora_period)
  {
   lora_period = random(5000, 7000);
    Serial.printf("new period: %lu\r\n", lora_period);
    lora_ticks = millis();
    sendLoRa();
    bpm = 0;
    contactsts = 0;
  }
}

void initGPS()
{
  Serial1.begin(115200, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);

  if (myGPS.begin(Serial1))
  {
    Serial.print("GPS MODULE RESPONDS!");
  }
  else
  {
    Serial.print("GPS MODULE DOES NOT RESPOND!");
  }
}

void initLoRa()
{
  Serial.println("Initializing LoRa....");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  int result = LoRa.begin(LORA_BAND);
  if (result != 1)
  {
    Serial.println("Failed to start LoRa network!");
    for (;;)
      ;
  }
  Serial.println("LoRa initialized");
}
void initHB()
{
  Wire.begin();
  Wire.setClock(400000UL);
  heartbeat_config();
}

void getGPSData()
{
 bool b=myGPS.powerSaveMode(false,100);
Serial.printf("GPS Power save mode off: %s\r\n",b?"OK":"FAIL");
  longitude = myGPS.getLongitude(100UL);
  latitude = myGPS.getLatitude(100UL);
  altitude = myGPS.getAltitude(100UL);
  SIV = myGPS.getSIV();
   b = myGPS.powerSaveMode(true,100);
  Serial.printf("GPS Power save mode on: %s\r\n",b?"OK":"FAIL");


}
char buffer[192] = {0};

void sendLoRa()
{
  memset(myPacket.data, 0, sizeof(myPacket.data));
  snprintf(myPacket.data, sizeof(myPacket.data), "{\"id\":%d,\"hb\":%d,\"contsts\":%d,\"longitude\":%li,\"latitude\":%li,\"altitude\":%li,\"sat\":%d,\"millis\":%lu}", devid, bpm, contactsts, longitude, latitude, altitude, SIV, millis());
  crc.restart();
  crc.add((uint8_t *)myPacket.data, sizeof(myPacket.data));
  myPacket.crc = crc.getCRC();
  Serial.printf("%s, CRC: %.4x\r\n", myPacket.data, myPacket.crc);
  LoRa.beginPacket();
  LoRa.write((uint8_t *)&myPacket, sizeof(myPacket));
  LoRa.endPacket();
}