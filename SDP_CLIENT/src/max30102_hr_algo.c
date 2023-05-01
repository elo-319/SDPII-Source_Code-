#include "max30102_hr_algo.h"
#include "math.h"

uint8_t spo2_table[184] =
    {95, 95, 95, 96, 96, 96, 97, 97, 97, 97, 97, 98, 98, 98, 98, 98, 99, 99, 99, 99,
     99, 99, 99, 99, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
     100, 100, 100, 100, 99, 99, 99, 99, 99, 99, 99, 99, 98, 98, 98, 98, 98, 98, 97, 97,
     97, 97, 96, 96, 96, 96, 95, 95, 95, 94, 94, 94, 93, 93, 93, 92, 92, 92, 91, 91,
     90, 90, 89, 89, 89, 88, 88, 87, 87, 86, 86, 85, 85, 84, 84, 83, 82, 82, 81, 81,
     80, 80, 79, 78, 78, 77, 76, 76, 75, 74, 74, 73, 72, 72, 71, 70, 69, 69, 68, 67,
     66, 66, 65, 64, 63, 62, 62, 61, 60, 59, 58, 57, 56, 56, 55, 54, 53, 52, 51, 50,
     49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 31, 30, 29,
     28, 27, 26, 25, 23, 22, 21, 20, 19, 17, 16, 15, 14, 12, 11, 10, 9, 7, 6, 5,
     3, 2, 1};

void MAFilter_init(struct MAFilter *mflt)
{
    mflt->nextslot = 0;
}
int16_t MAFilter_filter(struct MAFilter *mflt, int16_t value)
{
    mflt->buffer[mflt->nextslot] = value;
    mflt->nextslot = (mflt->nextslot + 1) % NSLOT;
    int16_t total = 0;
    for (int i = 0; i < NSLOT; ++i)
        total += mflt->buffer[i];
    return total / NSLOT;
}


void DCFilter_init(struct DCFilter *dcflt)
{
    dcflt->sample_avg_total = 0;
}
int16_t DCFilter_filter(struct DCFilter *dcflt, int32_t sample)
{
    dcflt->sample_avg_total += (sample - dcflt->sample_avg_total / NSAMPLE);
    return (int16_t)(sample - dcflt->sample_avg_total / NSAMPLE);
}

int32_t DCFilter_avgDC(struct DCFilter *dcflt)
{
    return dcflt->sample_avg_total / NSAMPLE;
}


void Pulse_init(struct Pulse *pls, struct DCFilter *dcflt, struct MAFilter *mflt)
{
    pls->dc = dcflt;
    pls->ma = mflt;
    pls->cycle_max = 20;
    pls->cycle_min = -20;
    pls->positive = false;
    pls->prev_sig = 0;
    pls->amplitude_avg_total = 0;
}

int16_t Pulse_dc_filter(struct Pulse *pls, int32_t sample)
{
    return DCFilter_filter(pls->dc, sample);
}
int16_t Pulse_ma_filter(struct Pulse *pls, int16_t sample)
{
    return MAFilter_filter(pls->ma, sample);
}
int32_t Pulse_avgDC(struct Pulse *pls)
{
    return DCFilter_avgDC(pls->dc);
}
int16_t Pulse_avgAC(struct Pulse *pls)
{
    return pls->amplitude_avg_total / 4;
}
bool Pulse_isBeat(struct Pulse *pls, int16_t signal)
{
    bool beat = false;
    // while positive slope record maximum
    if (pls->positive && (signal > pls->prev_sig))
        pls->cycle_max = signal;
    // while negative slope record minimum
    if (!pls->positive && (signal < pls->prev_sig))
        pls->cycle_min = signal;
    //  positive to negative i.e peak so declare beat
    if (pls->positive && (signal < pls->prev_sig))
    {
        int amplitude = pls->cycle_max - pls->cycle_min;
        if (amplitude > 20 && amplitude < 3000)
        {
            beat = true;
            pls->amplitude_avg_total += (amplitude - pls->amplitude_avg_total / 4);
        }
        pls->cycle_min = 0;
        pls->positive = false;
    }
    // negative to positive i.e valley bottom
    if (!pls->positive && (signal > pls->prev_sig))
    {
        pls->cycle_max = 0;
        pls->positive = true;
    }
    pls->prev_sig = signal; // save signal
    return beat;
}