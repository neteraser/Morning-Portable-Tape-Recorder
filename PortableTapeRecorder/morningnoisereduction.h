#pragma once

void NoiseRed48khzGetProfile(const float* buffer, int length);

void NoiseRed48khzProcess(float* samples, int length, float hardness);