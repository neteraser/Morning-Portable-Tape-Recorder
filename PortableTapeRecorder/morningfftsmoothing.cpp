
#include "morningfftsmoothing.h"
#include "recorder.h"
#include "log.h"
#include <math.h>
#include "AudioFFT.h"

using namespace audiofft;

AudioFFT fft;

float fft_real[FFTSMOOTHLEN];
float fft_imag[FFTSMOOTHLEN];

float fft_spec_real[FFTSMOOTHLEN];
float fft_spec_imag[FFTSMOOTHLEN];

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

void FFTBuildMediumSpectrum(const float* buffer, int length, int spectrum_length)
{
	ZeroMemory(fft_spec_real, spectrum_length * sizeof(float));
	ZeroMemory(fft_spec_imag, spectrum_length * sizeof(float));

	fft.init(spectrum_length);

	int count = 0;

	for (int i = 0; i < length - spectrum_length; i += spectrum_length)
	{
		fft.fft(buffer + i, fft_real, fft_imag);
		for (int j = 0; j < spectrum_length; ++j)
		{
			fft_spec_real[j] += fft_real[j];
			fft_spec_imag[j] += fft_imag[j];
		}
		//fft.ifft(buffer + i, greal, gimag);

		count += 1;
	}

	FFTSmoothNormalizeBuffer(fft_spec_real, spectrum_length);
	FFTSmoothNormalizeBuffer(fft_spec_imag, spectrum_length);

	for (int j = 0; j < spectrum_length; ++j)
	{
		fft_spec_real[j] *= (float)count;
		fft_spec_imag[j] *= (float)count;
	}
}