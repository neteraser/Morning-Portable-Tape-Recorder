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
};

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