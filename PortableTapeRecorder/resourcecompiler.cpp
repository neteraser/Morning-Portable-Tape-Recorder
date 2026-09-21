
#include "resourcecompiler.h"

#include <fstream>

PrecompiledResource* precompiledresources[256];
int precompiledResourcesNumber;

bool RetrieveSoundFromCode(const char* filename, Sound& snd)
{
	bool found = false;
	for (int i = 0; i < precompiledResourcesNumber; ++i)
	{
		PrecompiledResource* rc = precompiledresources[i];
		if (rc->filename == filename)
		{
			PrecompiledSound* rcsnd = reinterpret_cast<PrecompiledSound*>(rc);
			snd = rcsnd->snd;
			found = true;
		}
	}
	return found;
}

static int bakedResourcesNumber = 0;

bool BakeSoundToCode(Sound& snd, const char* filename)
{
#ifndef DOBAKERESOURCES
	return false;
#endif
	char resfilename[256];
	sprintf(resfilename, "precompiledresources/resource%04i.cpp", bakedResourcesNumber);

	std::ofstream fout;
	fout.open(resfilename, std::ios_base::out | std::ios_base::trunc);

	fout << "#include \"../resourcecompiler.h\"" << std::endl;
	fout << std::endl;
	fout << "static int RES_LEN = " << snd.length << "; " << std::endl;
	fout << std::endl;
	fout << "static float RES_DATA [" << snd.length << "] = { " << std::endl;
	for (int i = 0; i < snd.length; ++i)
	{
		char fvbuf[256];
		sprintf(fvbuf, "%1.96ff", snd.sampledata[i]);
		fout << fvbuf << ", " << std::endl;
	}
	fout << "}; " << std::endl;
	fout << std::endl;
	fout << "PrecompiledSound res" << bakedResourcesNumber++ << "( \"" << filename << "\", RES_LEN, RES_DATA);" << std::endl;
	fout << std::endl;
	fout.close();
	return true;
}

void BuildResourceLink(const char* filename)
{
	if (bakedResourcesNumber <= 0)
		return;

	std::ofstream fout;
	fout.open(filename, std::ios_base::out | std::ios_base::trunc);
	for (int i = 0; i < bakedResourcesNumber; ++i)
	{
		char resfilename[256];
		sprintf(resfilename, "resource%04i.cpp", i);
		fout << "#include \"" << resfilename << "\"" << std::endl;
	}
	fout.close();
}