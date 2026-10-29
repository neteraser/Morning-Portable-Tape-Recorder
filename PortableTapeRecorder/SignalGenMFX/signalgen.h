#pragma once

#include "morningfx.h"

class SignalGenMFX: public MorningFX
{
public:
	virtual const char* GetEffectName();

	virtual void LoadEffect();

	virtual void DrawEffect();

	virtual int ProcessEffect(float* buffer, int length, float mix);

	virtual int GetEffectType() { return MFXTYPE_GENERATOR; }
};