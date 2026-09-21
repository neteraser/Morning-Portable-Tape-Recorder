#pragma once

#include "main.h"

#include <string>

class PrecompiledResource;

extern PrecompiledResource* precompiledresources[256];
extern int precompiledResourcesNumber;

class PrecompiledResource
{
public:
	std::string filename;

	PrecompiledResource(std::string fn) {
		filename = fn;
		precompiledresources[precompiledResourcesNumber++] = this;
	}
};

class PrecompiledSound : public PrecompiledResource
{
public:
	
	Sound snd;

	PrecompiledSound(std::string fn, int len, float* data)
		: PrecompiledResource(fn)
	{
		snd.length = len;
		snd.sampledata = data;
		snd.snd = 0;
	}
};

bool RetrieveSoundFromCode(const char* filename, Sound& out);

bool BakeSoundToCode(Sound& in, const char* filename);

void BuildResourceLink(const char* filename);

/////////////////////////////////////////////////////////////////////

/// add resources 

/////////////////////////////////////////////////////////////////////

//#include "precompiledresources/beepforme_precompiled.cpp"
