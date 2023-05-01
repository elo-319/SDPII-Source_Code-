#include "Arduino.h"
#include <Wire.h>
#include <SPI.h>
#include <LoRa.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include "CRC16.h"
#include <esp_task_wdt.h>

struct packet
{
  char data[192];
  uint16_t crc;
} myPacket;

CRC16 crc;
#define LORA_MISO 19
#define LORA_CS 18
#define LORA_MOSI 27
#define LORA_SCK 5
#define LORA_RST 14
#define LORA_IRQ 26
#define LORA_BAND 868E6

#define LORA_BUFFER_SIZE sizeof(packet)
uint8_t LORA_BUFFER[LORA_BUFFER_SIZE];
StaticJsonDocument<1024> json;
const char *WIFI_SSID = "Elshan";
const char *WIFI_PASSWORD = "12345678";
const char *MQTT_USERNAME = "sdpdevice";
const char *MQTT_PASSWORD = "12345678";

const char *THINGSBOARD_SERVER = "thingsboard.cloud";
uint16_t THINGSBOARD_PORT = 1883U;
const char *THINGSBOARD_PUBLISH_TELEMETRY_URL = "v1/devices/me/telemetry";
const char *THINGSBOARD_PUBLISH_ATTRIBUTE_URL = "v1/devices/me/attributes";
const char *THINGSBOARD_PUBLISH_ATTRIBUTE_REQUEST_URL = "v1/devices/me/attributes";
const char *THINGSBOARD_SUBSCRIBE_ATTRIBUTE_URL = "v1/devices/me/attributes/response/+";
uint32_t THINGSBOARD_ATTRIBUTE_REQUEST_ID;

bool wifi_connected = false;
bool mqtt_connected = false;
bool mqtt_subscribed = false;
bool mqtt_publish_status = false;

uint32_t wifi_connect_counter = 0;
uint32_t wifi_max_retry_attempt = 10;

uint32_t mqtt_publish_time = 0xffffff;
uint32_t mqtt_publish_period = 5000;

long latitude = 0, longitude = 0, altitude = 0;
uint8_t SIV = 0;
uint8_t contactsts, bpm;

uint32_t id = 0, oldid = 1;
WiFiClient espClient;
PubSubClient mqttClient(espClient);
bool newdata = false;
char DEVICE_ID_BUFFER[10];
bool connect_wifi();
void initLoRa();
bool sendData = false;
void LoRaTask(void *pvParameters);
void WiFiTask(void *pvParameters);
void setup()
{
  Serial.begin(115200);
  initLoRa();
  wifi_connected = connect_wifi();
  if (wifi_connected)
  {
    Serial.printf("Connected to %s IP: ", WIFI_SSID);
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.printf("Could not connect to wifi!\r\n");
  }
  mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  mqttClient.setBufferSize(1024);
    esp_task_wdt_init(3, true); //enable panic so ESP32 restarts

}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    mqtt_connected = false;
    mqtt_subscribed = false;
    Serial.printf("WIFI disconnected, connecting: ");
    wifi_connected = connect_wifi();
    Serial.printf("%s\r\n", wifi_connected ? "OK" : "FAIL");
  }
  if (newdata && myPacket.crc != 0)
  {
    crc.restart();
    crc.add((uint8_t *)myPacket.data, sizeof(myPacket.data));
    uint16_t compcrc = crc.getCRC();
    if (compcrc == myPacket.crc)
    {
      newdata = false;
      sendData = true;
    }
    else
    {
      newdata = false;
      sendData = false;
    }
  }
  if (wifi_connected)
  {
    if (sendData)
    {
      json.clear();
      packet myPacket2;
      memcpy(myPacket2.data,myPacket.data,sizeof(myPacket.data));
      DeserializationError err = deserializeJson(json, myPacket.data);
      if (err == DeserializationError::Ok)
      {
        id = json["id"].as<uint32_t>();
        char idbuf[16] = {0};
        snprintf(idbuf, sizeof(idbuf), "%lu", id);
        mqtt_connected = mqttClient.connect(idbuf, MQTT_USERNAME, MQTT_PASSWORD);
        if (mqtt_connected)
        {
          Serial.printf("MQTT Connected, publishing: %s\r\n",myPacket2.data);
          bool publish_status = mqttClient.publish(THINGSBOARD_PUBLISH_TELEMETRY_URL, myPacket2.data);
          if (publish_status)
          {
            Serial.printf("Publish done\r\n");
          }
          else
          {
            Serial.printf("Publish fail\r\n");
          }
          mqttClient.disconnect();
        }
        else
        {
          Serial.printf("MQTT Connect fail\r\n");
        }
      }
      else{
        Serial.printf("JSON Deserialization fail\r\n");
      }
      sendData = false;
    }
  }
esp_task_wdt_reset();
  mqttClient.loop();
}

bool connect_wifi()
{
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED)
  {
    wifi_connect_counter++;
    vTaskDelay(500);
    if (wifi_connect_counter > wifi_max_retry_attempt)
    {
      break;
    }
  }
  if (WiFi.status() == WL_CONNECTED)
  {
    wifi_connect_counter = 0;
    return true;
  }
  return false;
}

void onReceive(int packetSize)
{
  int pSize = packetSize;
  if (packetSize > sizeof(myPacket))
  {
    pSize = sizeof(myPacket);
  }
  if (!newdata)
  {
    newdata = true;
    memset(LORA_BUFFER, 0, LORA_BUFFER_SIZE);
    memset((uint8_t *)&myPacket, 0, LORA_BUFFER_SIZE);

    for (int i = 0; i < packetSize; i++)
    {
      LORA_BUFFER[i] = LoRa.read();
    }
    memcpy((uint8_t *)&myPacket, LORA_BUFFER, LORA_BUFFER_SIZE);
  }
}

void initLoRa()
{
  Serial.println("Initializing LoRa....");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  // Start LoRa using the frequency
  int result = LoRa.begin(LORA_BAND);
  if (result != 1)
  {
    Serial.println("Failed to start LoRa network!");
    for (;;)
      ;
  }
  Serial.println("LoRa initialized");
  LoRa.onReceive(onReceive);
  LoRa.receive();
}