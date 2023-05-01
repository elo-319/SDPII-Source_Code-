#include "max30102_lib.h"
#include <Wire.h>
struct max30102_fifo_data
{
    uint32_t red[STORAGE_SIZE];
    uint32_t IR[STORAGE_SIZE];
    uint8_t head;
    uint8_t tail;
} sense;


bool maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data)
{
    Wire.beginTransmission(0x57);
    Wire.write(uch_addr);
    Wire.write(uch_data);
    return Wire.endTransmission() == 0;

}
bool maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data)
{
  Wire.beginTransmission(0x57);
  Wire.write(uch_addr);
  Wire.endTransmission();

  Wire.requestFrom((uint8_t)0x57, (uint8_t)1); // Request 1 byte
  if (Wire.available())
  {
   *puch_data = Wire.read();
  }
return true;
}

bool maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led)
{
    uint32_t un_temp;
    unsigned char uch_temp;
    *pun_red_led = 0;
    *pun_ir_led = 0;
    char ach_i2c_data[6];

Wire.beginTransmission(0x57);
Wire.write(REG_FIFO_DATA);
Wire.endTransmission();
uint8_t reg_bytes = Wire.requestFrom(0x57,6);
if(!reg_bytes)return false;
for(int i = 0; i<6;i++){
    ach_i2c_data[i]=Wire.read();
}
    un_temp = (unsigned char)ach_i2c_data[0];
    un_temp <<= 16;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[1];
    un_temp <<= 8;
    *pun_red_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[2];
    *pun_red_led += un_temp;

    un_temp = (unsigned char)ach_i2c_data[3];
    un_temp <<= 16;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[4];
    un_temp <<= 8;
    *pun_ir_led += un_temp;
    un_temp = (unsigned char)ach_i2c_data[5];
    *pun_ir_led += un_temp;
    *pun_red_led &= 0x03FFFF; // Mask MSB [23:18]
    *pun_ir_led &= 0x03FFFF;  // Mask MSB [23:18]
    return true;
}

bool maxim_max30102_reset()
{
    if (!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x40))
    {
         Serial.printf("Failed to write %.2x to REG_MODE_CONFIG", 0x40);
        return false;
    }
    return true;
}
bool maxim_max30102_shutdown()
{
    if (!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x80))
    {
         Serial.printf("Failed to write %.2x to REG_MODE_CONFIG", 0x80);
        return false;
    }
    return true;
}
bool maxim_max30102_init()
{
    if (maxim_max30102_shutdown())
    {
        Serial.printf("MAX30102 shutdown done!\r\n");
    }
    else
    {
        Serial.printf("MAX30102 shutdown failed!\r\n");
    }
delay(500);

    if (maxim_max30102_reset())
    {
        Serial.printf("MAX30102 reset done!\r\n");
    }
    else
    {
        Serial.printf("MAX30102 reset failed!\r\n");
    }
    delay(500);
    uint8_t uch_dummy;
  

     if (!maxim_max30102_write_reg(REG_INTR_ENABLE_1, 0xc0)) // INTR setting
         return false;
     if (!maxim_max30102_write_reg(REG_INTR_ENABLE_2, 0x00))
         return false;
    if (!maxim_max30102_write_reg(REG_FIFO_WR_PTR, 0x00)) // FIFO_WR_PTR[4:0]
        return false;
    if (!maxim_max30102_write_reg(REG_OVF_COUNTER, 0x00)) // OVF_COUNTER[4:0]
        return false;
    if (!maxim_max30102_write_reg(REG_FIFO_RD_PTR, 0x00)) // FIFO_RD_PTR[4:0]
        return false;
    if (!maxim_max30102_write_reg(REG_FIFO_CONFIG, 0x4f)) // sample avg = 1, fifo rollover=false, fifo almost full = 17
        return false;
    if (!maxim_max30102_write_reg(REG_MODE_CONFIG, 0x03)) // 0x02 for Red only, 0x03 for SpO2 mode 0x07 multimode LED
        return false;
    if (!maxim_max30102_write_reg(REG_SPO2_CONFIG, 0x27)) // SPO2_ADC range = 4096nA, SPO2 sample rate (100 Hz), LED pulseWidth (400uS)
        return false;
    if (!maxim_max30102_write_reg(REG_LED1_PA, 0x17)) // Choose value for ~ 7mA for LED1
        return false;
    if (!maxim_max30102_write_reg(REG_LED2_PA, 0x17)) // Choose value for ~ 7mA for LED2
        return false;
    if (!maxim_max30102_write_reg(REG_PILOT_PA, 0x1f)) // Choose value for ~ 25mA for Pilot LED
        return false;
    return true;
}

void max30102_nextSample()
{
    if (max30102_data_available())
    {
        sense.tail++;
        sense.tail %= STORAGE_SIZE; // Wrap condition
    }
}
uint8_t max30102_data_available()
{
    int8_t numberOfSamples = sense.head - sense.tail;
    if (numberOfSamples < 0)
        numberOfSamples += STORAGE_SIZE;
    return (numberOfSamples);
}
uint32_t max30102_getRed(void)
{
    return (sense.red[sense.tail]);
}

// Report the next IR value in the FIFO
uint32_t max30102_getIR(void)
{
    return (sense.IR[sense.tail]);
}

uint16_t max30102_check(void)
{
    uint8_t readPointer = 0, writePointer = 0;
    maxim_max30102_read_reg(REG_FIFO_RD_PTR, &readPointer);
    maxim_max30102_read_reg(REG_FIFO_WR_PTR, &writePointer);
    uint32_t red_val = 0, ir_val = 0;
    int numberOfSamples = 0;
    if (readPointer != writePointer)
    {
        numberOfSamples = writePointer - readPointer;
        if (numberOfSamples < 0)
            numberOfSamples += 32;                 // Wrap condition
        int bytesLeftToRead = numberOfSamples * 6; // 3 bytes each for Red and IR
        while (bytesLeftToRead > 0)
        {
            sense.head++;               // Advance the head of the storage struct
            sense.head %= STORAGE_SIZE; // Wrap condition
            maxim_max30102_read_fifo(&red_val, &ir_val);
            sense.IR[sense.head] = ir_val;
            sense.red[sense.head] = red_val;
            bytesLeftToRead -= 6;
        }
    }
    return (numberOfSamples);
}