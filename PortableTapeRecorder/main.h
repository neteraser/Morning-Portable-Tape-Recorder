#pragma once

//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <d3d9.h>
#include <d3dx9.h>
#include <windows.h>

#define  DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#include "bass/bass.h"

struct Sound {
	HSAMPLE snd;
	int length;
	float* sampledata;
};

struct Picture
{
	IDirect3DTexture9* texture;
	int width;
	int height;
	bool sendclick;
};

#define CURRENTSETTINGSVERSION (1)

struct CurrentSettings
{
	int structversion;
	int coloration;
	float rq;
	int audiolibrary;
	bool delaymode;
	char device[256];
	char recdevice[256];
};

extern CurrentSettings cursettings;


#ifdef _DEBUG
#define DOBAKERESOURCES
#define DEBUGSCREEN
#endif
#ifdef NDEBUG
#undef DOBAKERESOURCES
#endif
//#define DOBAKERESOURCES
// baking resources to c++ code does work as a console app trick, such apps are loading much faster

extern HWND mainWnd;
extern HINSTANCE mainInstance;

#include "morningfx.h"

class MyAppInterface : public MorningAppInterface
{
	unsigned curclr; // for draw string
public:
	MyAppInterface() { curclr = 0xffffffff;  }

	virtual int LoadPicture(const char* filename);
	virtual void DrawPicture(int n, float x, float y, float w, float h);

	virtual int GetWindowWidth();
	virtual int GetWindowHeight();

	virtual bool DrawButton(int* pics, float x, float y);

	virtual void DrawString(const char* str, float l, float t, float r, float b);

	virtual void ApplyEffect(MorningFX* fx, bool longorshotop);
	virtual void CloseEffect();
	virtual void ReportEffectProgress(float progress);

	virtual bool KeyPressed(int k);
	virtual bool KeyTrigged(int k);

	virtual bool MouseL();
	virtual bool MouseClick();
	virtual bool MouseDoubleClick();
	virtual int MouseX();
	virtual int MouseY();
	virtual int MouseMoveX();
	virtual int MouseMoveY();

	virtual bool MouseInBox(int x, int y, int w, int h);

	virtual void SetColor(int r, int g, int b, int a);
	virtual void ResetColor();
};

extern MyAppInterface appinterface;