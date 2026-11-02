// MorningHiddenAudioProcessing.cpp 
#include "MorningHidden.h"

#include <math.h>
#include <cmath>

__inline double DigitalToAnalog(double volume)
{
	double analog;
	analog = log(std::abs(volume)) / 4.5;// / 100.0f;
	if (analog < -1.0)
		analog = -1.0;
	analog = 1.0 + analog;
	if (volume < 0)
		analog = -analog;
	return analog;
}

__inline double AnalogToDigital(double volume)
{
	double digital;
	digital = (std::pow(10.0, std::abs(volume) * 4.5));// -1.0;
	if (volume < 0)
		digital = -digital;
	return digital / 31500.0;
}

float adcd00 = 56.0f / (220.0f + 56.0f);
float adcd01 = 47.0f / (470.0f + 47.0f);

float adcd10 = 470.0f / (470.0f + 470.0f);
float adcd11 = 330.0f / (1000.0f + 330.0f);

float adcd20 = 16.0f / (68.0f + 16.0f);
float adcd21 = 22.0f / (68.0f + 22.0f);

#define DIVGREATER( a, b ) ( (a) < (b) ? (a) / (b) : (b) / (a) )

float adcdmult[3] = { DIVGREATER(adcd00, adcd01), DIVGREATER(adcd10, adcd11), DIVGREATER(adcd20, adcd21) };

float HiddenProcessSampleByColor_fn(float in, float mix, int clr)
{
	double vcolored = in;
	vcolored = DigitalToAnalog(vcolored);
	vcolored *= adcdmult[clr];
	vcolored = AnalogToDigital(vcolored);

	float vmix = in * (1.0f - mix * 0.7f) + (float)vcolored * mix * 0.7f;

	return vmix;
}

float GetColorMultiplicator_fn(int clr)
{
	return adcdmult[clr];
}

float (*HiddenProcessSampleByColor)(float, float, int) = HiddenProcessSampleByColor_fn;
float (*GetColorMultiplicator)(int) = GetColorMultiplicator_fn;