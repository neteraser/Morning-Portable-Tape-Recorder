
#define MFXAPI __declspec(dllimport)

#include "morningfx.h"
#include <Windows.h>
#include "main.h"
#include "recorder.h"
#include "xml/tinyxml.H"
#include <vector>

MorningFX* mfx[64] = { 0 };
int mfxNumber = 0;

void MorningFX::SetAppInterface(MorningAppInterface* appi)
{
	app = appi;
}


std::vector<HINSTANCE> loadedlibs;

void LoadMFXs(const char* xmlfile)
{
	TiXmlDocument doc;
	if (doc.LoadFile("mfxlist.xml"))
	{
		TiXmlElement* elem = doc.RootElement();

		TiXmlElement* temp;

		temp = elem->FirstChildElement("mfx");

		while (temp)
		{

			const char* dllname = temp->Attribute("dll");

			if (dllname)
			{
				HINSTANCE hlib;
				hlib = LoadLibrary(dllname);
				if (hlib != 0)
				{
					MFXFINDFUNC findfunc = (MFXFINDFUNC)GetProcAddress(hlib, "GetMyMFX");
					if (findfunc != 0) 
					{
						MorningFX* fx = findfunc();
						if (fx != 0)
						{
							fx->SetAppInterface(&appinterface);
							fx->LoadEffect();
							mfx[mfxNumber++] = fx;
						}
					}
				}
				loadedlibs.push_back(hlib);
			}

			temp = temp->NextSiblingElement("mfx");
		}
	}
}

void DrawMFX(int mfxn)
{
	mfx[mfxn]->DrawEffect();
}

void FreeMFXs()
{
	for (int i = 0; i < loadedlibs.size(); ++i)
	{
		FreeLibrary(loadedlibs[i]);
	}
	loadedlibs.clear();
}