#ifndef MAX30102_LIB_HR_ALGO_
#define MAX30102_LIB_HR_ALGO_
#ifdef __cplusplus
extern "C" {
#endif
#define NSAMPLE 24
#define NSLOT 4
#include "inttypes.h"
#include "stdbool.h"
extern  uint8_t spo2_table[184];

struct DCFilter
{
    int32_t sample_avg_total;
};

struct MAFilter
{
    uint8_t nextslot;
    int16_t buffer[NSLOT];
};

struct Pulse
{
    struct DCFilter *dc;
    struct MAFilter *ma;
    int16_t amplitude_avg_total;
    int16_t cycle_max;
    int16_t cycle_min;
    bool positive;
    int16_t prev_sig;
};


void MAFilter_init(struct MAFilter *mflt);
int16_t MAFilter_filter(struct MAFilter *mflt, int16_t value);
void DCFilter_init(struct DCFilter *dcflt);
int16_t DCFilter_filter(struct DCFilter *dcflt, int32_t sample);
int32_t DCFilter_avgDC(struct DCFilter *dcflt);
void Pulse_init(struct Pulse *pls, struct DCFilter *dcflt, struct MAFilter *mflt);
int16_t Pulse_dc_filter(struct Pulse *pls, int32_t sample);
int16_t Pulse_ma_filter(struct Pulse *pls, int16_t sample);
int32_t Pulse_avgDC(struct Pulse *pls);
int16_t Pulse_avgAC(struct Pulse *pls);
bool Pulse_isBeat(struct Pulse *pls, int16_t signal);
#ifdef __cplusplus
}
#endif
#endif