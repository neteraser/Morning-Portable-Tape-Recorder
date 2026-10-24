#pragma once

#define FFTSMOOTHLEN 1024

extern float fft_spec_real[FFTSMOOTHLEN];
extern float fft_spec_imag[FFTSMOOTHLEN];

void FFTBuildMediumSpectrum(const float* buffer, int length, int spectrum_length);