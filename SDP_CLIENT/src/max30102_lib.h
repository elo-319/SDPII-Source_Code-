#ifndef MAX30102_LIB_H_
#define MAX30102_LIB_H_
#include <Arduino.h>

#define STORAGE_SIZE 3

#define I2C_WRITE_ADDR 0x57 // 7-bit version of the above
#define I2C_READ_ADDR 0x57  // 7-bit version of the above

#define REG_INTR_STATUS_1 0x00
#define REG_INTR_STATUS_2 0x01
#define REG_INTR_ENABLE_1 0x02
#define REG_INTR_ENABLE_2 0x03
#define REG_FIFO_WR_PTR 0x04
#define REG_OVF_COUNTER 0x05
#define REG_FIFO_RD_PTR 0x06
#define REG_FIFO_DATA 0x07
#define REG_FIFO_CONFIG 0x08
#define REG_MODE_CONFIG 0x09
#define REG_SPO2_CONFIG 0x0A
#define REG_LED1_PA 0x0C
#define REG_LED2_PA 0x0D
#define REG_PILOT_PA 0x10
#define REG_MULTI_LED_CTRL1 0x11
#define REG_MULTI_LED_CTRL2 0x12
#define REG_TEMP_INTR 0x1F
#define REG_TEMP_FRAC 0x20
#define REG_TEMP_CONFIG 0x21
#define REG_PROX_INT_THRESH 0x30
#define REG_REV_ID 0xFE
#define REG_PART_ID 0xFF
struct max30102_fifo_data;
bool maxim_max30102_init();
bool maxim_max30102_read_fifo(uint32_t *pun_red_led, uint32_t *pun_ir_led);
bool maxim_max30102_write_reg(uint8_t uch_addr, uint8_t uch_data);
bool maxim_max30102_read_reg(uint8_t uch_addr, uint8_t *puch_data);
bool maxim_max30102_reset(void);
bool maxim_max30102_read_temperature(int8_t *integer_part, uint8_t *fractional_part);
bool maxim_max30102_shutdown();
uint8_t max30102_data_available();
void max30102_nextSample();
uint32_t max30102_getRed();
uint32_t max30102_getIR();
uint16_t max30102_check(void);
#endif