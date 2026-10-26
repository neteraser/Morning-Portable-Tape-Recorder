#pragma once

#define FFTLEN 1024
#define FFTCOMPLEXLEN ((FFTLEN)/2 + 1)
#define FFTLENTOCOMPLEX(x) ((x)/2 + 1)
#define FFTDSPLEN 64

extern float fft_spec_real[FFTLEN];
extern float fft_spec_imag[FFTLEN];

void FFTBuildMediumSpectrum(const float* buffer, int length, int spectrum_len, bool fordrawing = false);

void FFTNormalize(float* buffer, int length, int spectrum_len, float hardness);