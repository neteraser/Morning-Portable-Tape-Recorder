
#include "morningfftsmoothing.h"
#include "recorder.h"
#include "log.h"
#include <math.h>
#include "AudioFFT.h"

using namespace audiofft;

float fft_real[FFTLEN];
float fft_imag[FFTLEN];

float fft_spec_real[FFTLEN];
float fft_spec_imag[FFTLEN];

void FFTSmoothNormalizeBuffer(float* buffer, int length)
{
	float fmax = 0;
	for (int i = 0; i < length; ++i)
	{
		if (fabs(buffer[i]) > fmax)
			fmax = fabs(buffer[i]);
	}
	for (int i = 0; i < length; ++i)
		buffer[i] /= fmax;
}

void FFTBuildMediumSpectrum(const float* buffer, int length, int spectrum_len, bool fordrawing)
{
	ZeroMemory(fft_spec_real, spectrum_len * sizeof(float));
	ZeroMemory(fft_spec_imag, spectrum_len * sizeof(float));

	AudioFFT fft;

	fft.init(spectrum_len);

	int count = 0;

	for (int i = 0; i < length - spectrum_len; i += spectrum_len)
	{
		fft.fft(buffer + i, fft_real, fft_imag);
		for (int j = 0; j < FFTLENTOCOMPLEX(spectrum_len); ++j)
		{
			fft_spec_real[j] += fft_real[j];
			fft_spec_imag[j] += fft_imag[j];
		}
		//fft.ifft(buffer + i, greal, gimag);

		count += 1;
	}

	FFTSmoothNormalizeBuffer(fft_spec_real, FFTLENTOCOMPLEX(spectrum_len));
	FFTSmoothNormalizeBuffer(fft_spec_imag, FFTLENTOCOMPLEX(spectrum_len));

	/*if (fordrawing)
	{
		for (int j = 0; j < spectrum_len; ++j)
		{
			fft_spec_real[j] *= (float)count;
			fft_spec_imag[j] *= (float)count;
		}
	}*/
}

void FFTNormalize(float* buffer, int length, int spectrum_len, float hardness)
{
	FFTBuildMediumSpectrum(buffer, length, FFTLEN, false);

	AudioFFT fft;

	fft.init(spectrum_len);
	
	float* out = (float*)malloc(sizeof(float) * length);

	CopyMemory(out, buffer, sizeof(float) * length);

	for (int j = 0; j < length - spectrum_len; j += 1)
	{
		fft.fft(out + j, fft_real, fft_imag);

		for (int i = 0; i < FFTLENTOCOMPLEX(spectrum_len); ++i)
		{
			fft_real[i] *= fft_spec_real[i];
			fft_imag[i] *= fft_spec_imag[i];
		}

		fft.ifft(out + j, fft_real, fft_imag);

		RecorderReportProgress((float)(((double)j / (double)length)));
	}

	for (int i = 0; i < length; ++i)
	{
		buffer[i] = out[i] * hardness + (1.0f - hardness) * buffer[i];
	}

	free(out);
}