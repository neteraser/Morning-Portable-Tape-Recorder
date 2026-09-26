#pragma once

extern float (*HiddenProcessSampleByColor)(float in, float qfx, int clr);

extern float (*GetColorMultiplicator)(int clr);

#define BEGINHIDDENCODE(func) if (func != 0) {
#define ENDHIDDENCODE }