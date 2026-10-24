
#include "morningnoisereduction.h"
#include "recorder.h"
#include "log.h"
#include <math.h>

#define NRSPECTRUMLEN 4096

static float noiseprofile[NRSPECTRUMLEN];
static float noisespectrum[NRSPECTRUMLEN];
static float spectrum[NRSPECTRUMLEN];

int clamplen = 0;

float NRNormalizeBuffer(float* buffer, int length)
{
	float outmax = 0.0f;
	for (int i = 0; i < length; ++i)
	{
		float fv = fabs(buffer[i]);
		if (fv > outmax)
			outmax = fv;
	}
	for (int i = 0; i < length; ++i)
	{
		buffer[i] /= outmax;
	}
	return outmax;
}

int NRBuildMediumSpectrum(const float* buffer, int start, int bufferlength, float* out, int length)
{
	int j = start;

	ZeroMemory(out, length * sizeof(float));

	int chunks = 0;

	while (j < start+bufferlength)
	{
//		if (start + bufferlength - j < NRSPECTRUMLEN-1)
//			break;

		for (int i = 0; i < length; ++i)
		{
			int i0, i1, i2, i3;
			i0 = j + i - 2;
			if (i0 < 0)
				i0 = 0;
			i1 = j + i - 1;
			if (i1 < 0)
				i1 = 0;
			i2 = j + i;
			i3 = j + i + 1;
			if (i3 >= clamplen)
				i3 = clamplen - 1;

			float t = ((float)i / (float)(length-1));
			//t *= 2;
			float sp = CatmullRom1D(t, buffer[i0], buffer[i1], buffer[i2], buffer[i3]);

			out[i] += sp;//(sp + out[i]) / 2.0f;
		}
		j += length;
		chunks += 1;
	}
	for (int i = 0; i < length; ++i)
	{
		out[i] /= (float)chunks;
	}
	//return chunks;
	//NRNormalizeBuffer(out, length);

	return 0;
}

void NoiseRed48khzGetProfile(const float* buffer, int length)
{
	NRBuildMediumSpectrum(buffer, 0, length, noiseprofile, NRSPECTRUMLEN);
	NRNormalizeBuffer(noiseprofile, NRSPECTRUMLEN);

	for (int i = 0; i < NRSPECTRUMLEN; ++i)
	{
		noiseprofile[i] = -noiseprofile[i];
	}
}

float spectrum_norm[NRSPECTRUMLEN];

void NRInduceSpectrumNoise(float* spec, float* noisespec, int length, float hardness, float hardness2)
{
	CopyMemory(spectrum_norm, spec, length * sizeof(float));

	float spmax = NRNormalizeBuffer(spectrum_norm, length);

	for (int i = 0; i < length; ++i)
	{
		float damping = 1.0f - (float)i / (float)(length-1);
		float spmix = noisespec[i] * damping * hardness + spectrum_norm[i] * (1.0f - hardness);

		noisespec[i] = spec[i] * hardness2 + (1.0f - hardness2) * spmix * spmax;// *hardness2 + spectrum_norm[i] * (1.0f - hardness2);// *(1.0f - hardness2) + spec[i] * hardness2;
	}
}

__inline void NRHurt(float* out, const float* buffer, int start, int i, float hardness, float hardness2)
{
	if (i < 0)
		i = 0;
	if (i > clamplen)
		i = clamplen;

	int i0, i1, i2, i3;
	i0 = start + i - 2;
	if (i0 < 0)
		i0 = 0;
	i1 = start + i - 1;
	if (i1 < 0)
		i1 = 0;
	i2 = start + i;
	i3 = start + i + 1;
	if (i3 > clamplen)
		i3 = clamplen;

	//float out = buffer[i0] * hardness;
	//buffer[i] = out;

	//out[i] -= out[i] * hardness;
	//out[i] *= hardness;
	//out[i0] *= (1.0f + hardness * 0.3f);

	//out[i] += out[i] * hardness;
	//float outq = (1.0f - out[i] * out[i]) * out[i];
	//out[i1] += out[i1] * hardness;

	//out[i0] += out[i0] * hardness * hardness2;

	//out[i2] += out[i2] * hardness * 0.3f;

	//out[i] *= (1.0f + hardness);
	//out[i2] *= (1.0f + hardness * 0.3f);
	//if (out[i] > 1.0f)
//		out[i] = 1.0f;
	//if (out[i] < -1.0f)
	//	out[i] = -1.0f;
	
	// I can't do it so far

	// lets try to do it
	float sp = CatmullRom1D((float)i / (float)(NRSPECTRUMLEN - 1), out[i0], out[i1], out[i2] * (1.0f + hardness), out[i3]);
	out[i] = sp * hardness + (1.0f - hardness) * out[i];
}

void NRDropGain(float* out, const float* buffer, const float* spec, int start, int length, float hardness, float hardness2)
{
	for (int i = 0; i < length; ++i)
	{
		int i0, i1, i2, i3;
		i0 = start + i - 2;
		if (i0 < 0)
			i0 = 0;
		i1 = start + i - 1;
		if (i1 < 0)
			i1 = 0;
		i2 = start + i;
		i3 = start + i + 1;
		if (i3 > clamplen)
			i3 = clamplen;

		double p0 = buffer[i1], p1 = buffer[i2];
		
		double sp = spec[i];

		// induce 
		if (p0 < p1)
		{
			if (sp < p0)
				sp = p0;
			if (sp > p1)
				sp = p1;
		}
		else
		{
			if (sp > p0)
				sp = p0;
			if (sp < p1)
				sp = p1;
		}

		int tt = InverseCatmullRom1DInArray(NRSPECTRUMLEN, sp, buffer[i0], p0, p1, buffer[i3]);

		int ti = tt;//(int)round(tt * (double)(length - 1));
		if (ti < 0)
			ti = 0;
		if (ti > length - 1)
			ti = length - 1;

		//if (i != ti)
		//{
		//	WriteToLog("remnoise val: %i", ti);
		//}

		//ti += j;
		//ti += start;

		//out[ti] *= hardness;
		NRHurt(out, buffer, start, ti, hardness, hardness2);
	}
}

void NRLowPassFilter(float* buffer, int length)
{
	const float frequency = 8000.0f;
	
	float x = tanf(REC_PI * frequency / 48000.0f);

	float prevInput = 0, prevOutput = 0;

	for (int i = 0; i < length; ++i)
	{
		float output = x * buffer[i] + x * prevInput - (x - 1) * prevOutput;
		output /= (x + 1);

		prevInput = buffer[i];
		prevOutput = output;
		
		buffer[i] = buffer[i] * 0.3f + output * 0.7f;
	}
}

void NRHighPassFilter(float* buffer, int length)
{
	const float Kf = /*150.0f / 48000.0f;*/ 0.0016666f;//0.003125f;
	float FilterAcc = 0.0f;

	for (int i = 0; i < length; ++i)
	{
		FilterAcc += Kf * (buffer[i]  - FilterAcc);
		buffer[i] = buffer[i] - FilterAcc;
	}
}

float* outbuffer = 0;

void NoiseRed48khzProcess(float* buffer, int length, float hardness)
{
	//NRLowPassFilter(buffer, length);

	outbuffer = (float*)malloc(length * sizeof(float));

	//ZeroMemory(outbuffer, length * sizeof(float));
	CopyMemory(outbuffer, buffer, length * sizeof(float));

	clamplen = length-1;

	int chunks = length / NRSPECTRUMLEN - 1;
	int lenclamped = chunks * NRSPECTRUMLEN;

	for (int p = 0; p < 16; ++p)
	{
		int j = 0;
		while (j < lenclamped)
		{
			NRBuildMediumSpectrum(buffer, j, NRSPECTRUMLEN, spectrum, NRSPECTRUMLEN);

			//NRNormalizeBuffer(spectrum, NRSPECTRUMLEN);

			CopyMemory(noisespectrum, noiseprofile, sizeof(float) * NRSPECTRUMLEN);

			NRInduceSpectrumNoise(spectrum, noisespectrum, NRSPECTRUMLEN, 0.002f, 0.6f);

			NRDropGain(outbuffer, buffer, noisespectrum, j, NRSPECTRUMLEN, 0.0003f, 0.1f);//0.99f - 1.0f / NRSPECTRUMLEN);

			j += NRSPECTRUMLEN;// / 16;
		}

		NRLowPassFilter(outbuffer, length);

		NRHighPassFilter(outbuffer, length);

		//NRNormalizeBuffer(outbuffer, length);

		for (int i = 0; i < length; ++i)
		{
			//float tmp;
			//tmp = outbuffer[i]; // source - signal
			buffer[i] = buffer[i] * (1.0f - hardness) + outbuffer[i] * hardness;// *0.8f;
		}
	}

	//CopyMemory(buffer, outbuffer, lenclamped * sizeof(float));

	//NRNormalizeBuffer(buffer, length);

	free(outbuffer);
}