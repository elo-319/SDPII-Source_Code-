#include "max30102_lib.h"
#include "max30102_hr_algo.h"

struct MAFilter IRMAFilter, redMAFilter, bpmMAFilter;
struct Pulse IRPulse, redPulse;
struct DCFilter IRDCFilter, redDCFilter;
bool beatRed, beatIR;
uint32_t lastBeat = 0;
int beatAvg = 0;
uint32_t _pwm_pin = 0;
uint32_t last_state_time = 0;
int state = 0;

void heartbeat_main(uint8_t *hbrate, uint8_t *contactsts)
{
max30102_check();
    uint32_t now = millis();
    if (max30102_data_available())
    {
        uint32_t irValue = max30102_getIR();
        uint32_t redValue = max30102_getRed();
        max30102_nextSample();
        if (irValue < 5000)
        {
            beatAvg = 0;
            *hbrate = beatAvg;
            *contactsts = 0;
                Serial.printf("millis: %lu, no finger\r\n",millis());
        }
        else
        {
            int16_t IR_signal, Red_signal;
            bool beatRed, beatIR;
            IR_signal = Pulse_dc_filter(&IRPulse, irValue);
            Red_signal = Pulse_dc_filter(&redPulse, redValue);
            beatRed = Pulse_isBeat(&redPulse, Pulse_ma_filter(&redPulse, Red_signal));
            beatIR = Pulse_isBeat(&IRPulse, Pulse_ma_filter(&IRPulse, IR_signal));
            *contactsts = 1;
            if (beatRed)
            {
                state = 1;
                uint32_t time_difference = now - lastBeat;
                if (time_difference == 0)
                    time_difference = 1;
                uint32_t btpm = 60000 / (time_difference);
                if (btpm > 0 && btpm < 200)
                    beatAvg = MAFilter_filter(&bpmMAFilter, (int16_t)btpm);
                lastBeat = now;
                *hbrate = beatAvg;
                Serial.printf("millis: %lu, bpm: %d\r\n",millis(),beatAvg);
            }
        }
    }
}
bool heartbeat_config()
{

    if (!maxim_max30102_init())
    {
        Serial.printf("MAX30102 initialization fail!\r\n");
        return false;
    }
    else
    {
        Serial.printf("MAX30102 initialized successfully!\r\n");
    }
    MAFilter_init(&IRMAFilter);
    MAFilter_init(&redMAFilter);
    MAFilter_init(&bpmMAFilter);
    DCFilter_init(&IRDCFilter);
    DCFilter_init(&redDCFilter);
    Pulse_init(&IRPulse, &IRDCFilter, &IRMAFilter);
    Pulse_init(&redPulse, &redDCFilter, &redMAFilter);

    return true;
}
