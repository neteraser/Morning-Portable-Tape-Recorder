#pragma once

#ifndef MFXAPI 
#define MFXAPI __declspec(dllexport)
#endif

#include <string>

class MorningFX;

class MorningAppInterface
{
public:

	virtual int LoadPicture(const char* filename) { return 0;  }

	virtual void DrawPicture(int pic, float x, float y, float w = 0.0f, float h = 0.0f) {}

	virtual int GetWindowWidth() { return 0; }
	virtual int GetWindowHeight() { return 0; }

	virtual bool DrawButton(int* pics, float x, float y) { return false;  }
	
	virtual void DrawString(const char* str, float l, float t, float r, float b) {}

	virtual bool MouseL() { return false; }
	virtual bool MouseClick() { return false;  }
	virtual bool MouseDoubleClick() { return false;  }
	virtual int MouseX() { return 0; }
	virtual int MouseY() { return 0;  }
	virtual int MouseMoveX() { return 0;  }
	virtual int MouseMoveY() { return 0;  }

	virtual bool KeyPressed(int k) { return false;  }
	virtual bool KeyTrigged(int k) { return false; }

	virtual void ApplyEffect(MorningFX* fx, bool longorshotop) {}
	virtual void CloseEffect() {}
	virtual void ReportEffectProgress(float progress) {} 

	virtual bool MouseInBox(int x, int y, int w, int h) { return false;  }

	virtual void SetColor(int r, int g, int b, int a) {}

	virtual void ResetColor() {}
};

#define MFXTYPE_EFFECT (0)
#define MFXTYPE_GENERATOR (1)

class MFXAPI MorningFX
{
public:

	//////////////// INTERFACE INSIDE //////////////

	MorningAppInterface* app;

	void SetAppInterface(MorningAppInterface* appi);

	//////////////// INTERFACE OUTSIDE //////////////

	virtual const char* GetEffectName() = 0;
	virtual void LoadEffect() {}
	virtual void DrawEffect() {}
	virtual int ProcessEffect(float* buffer, int length, float mix) { return 0; }

	virtual int GetEffectType() { return MFXTYPE_EFFECT;  }

	// depending on the effect type you get either the filled part 
	// of the buffer or the whole buffer (250s) in the ProcessEffect
};

typedef MorningFX* (*MFXFINDFUNC)();

extern MorningFX* mfx[64];
extern int mfxNumber;

void LoadMFXs(const char* xmldesc);

void DrawMFX(int mfxn);

void FreeMFXs();