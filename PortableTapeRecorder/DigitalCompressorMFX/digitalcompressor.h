#pragma once

#include "morningfx.h"

class DigitalDeadMeterMFX : public MorningFX
{
public:
	virtual const char* GetEffectName() { return "Dead meter"; }
	virtual void LoadEffect() {}
	virtual void DrawEffect() {}
	virtual int ProcessEffect(float* buffer, int length, float mix);
	virtual int GetEffectType() { return MFXTYPE_INSPECTOR;  }
};

class DigitalCompressorMFX : public MorningFX
{
public:
	virtual const char* GetEffectName();

	virtual void LoadEffect();

	virtual void DrawEffect();

	virtual int ProcessEffect(float* buffer, int length, float mix);

	virtual int GetEffectType() { return MFXTYPE_EFFECT; }
};