
#include "morningreverb.h"
#include "recorder.h"

#include <math.h>

struct filter
{
    float buf[4096];
    float fv;
    int fvi;
    int size;
};

float comb_process(filter& filter, float in, float feedback, float damping, float mult)
{
    float out = filter.buf[filter.fvi];
    filter.fv = out + (filter.fv - out) * damping;
    filter.buf[filter.fvi] = in * mult + filter.fv * feedback;
    filter.fvi += 1;
    if (filter.fvi >= filter.size)
        filter.fvi = 0;
    return out;
}

float allpass_process(filter& filter, float in, float mult)
{
    float out = filter.buf[filter.fvi];
    filter.buf[filter.fvi] = in * mult + out * 0.5f;
    filter.fvi += 1;
    if (filter.fvi >= filter.size)
        filter.fvi = 0;
    return out - in;
}

#define REVERB_COMB_LEN 16
#define REVERB_ALLPASS_LEN 8
//1720, 1785, 1850, 1925, 1995, 2075, 2145, 2205 
const int comb_lengths[REVERB_COMB_LEN] = { 820, 880, 950, 1000, 1050, 1125, 1200, 1300, 1350, 1400, 1550, 1650, 1750, 1850, 2050, 2250 };
const int allpass_lengths[REVERB_ALLPASS_LEN] = { 150, 250, 360, 440, 560, 600, 720, 800  };

filter combfilters[REVERB_COMB_LEN];
filter allpassfilters[REVERB_ALLPASS_LEN];

void init_filters()
{
    for (int i = 0; i < REVERB_COMB_LEN; ++i)
    {
        combfilters[i].fvi = 0;
        combfilters[i].size = comb_lengths[i];
        ZeroMemory(combfilters->buf, sizeof(float) * 4096);
        combfilters[i].fv = 0.0;
    }

    for (int i = 0; i < REVERB_ALLPASS_LEN; ++i)
    {
        allpassfilters[i].fvi = 0;
        allpassfilters[i].size = allpass_lengths[i];
        ZeroMemory(allpassfilters->buf, sizeof(float) * 4096);
        allpassfilters[i].fv = 0.0;
    }

}

void ProcessReverb48khz(float* buf, int samples, float qfx)
{
    init_filters();

    float feedback = 0.66f;
    float damping = 0.166f;

    for (int i = 0; i < samples; ++i)
    {
        float in = buf[i];
        float out = 0;

        float efficiency = 1.0f;

        for (int j = 0; j < REVERB_COMB_LEN; ++j)
        {
            out += comb_process(combfilters[j], in, feedback, damping, efficiency);
            efficiency *= 0.88f; // efficiency
        }

        float reflection = 1.0f;

        for (int j = 0; j < REVERB_ALLPASS_LEN; ++j)
        {
            out = allpass_process(allpassfilters[j], out, reflection);
            reflection *= 0.5f; // reflection
        }

        out *= 0.33f;

        float mix = in * (1.0f - qfx) + out * qfx;
        buf[i] = mix;
    }
}
