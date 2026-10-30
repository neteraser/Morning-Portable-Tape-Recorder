
#define _CRT_SECURE_NO_WARNINGS

#include "signalgen.h"
#include <dinput.h>

SignalGenMFX myeffect;

int back;
int px1;
int freqbut;
int button[2];
int close[2];

int popupX = 0, popupY = 0;
int popupWidth = 400, popupHeight = 200;

float frequencies[6] = { 150.0f, 300.0f, 600.0f, 800.0f, 1000.0f, 1200.0f };
const int maxfrequencies = 6;
int curfreq = 5;

void SignalGenMFX::LoadEffect()
{
	back = app->LoadPicture("plugins/signalgen/back.png");

	freqbut = app->LoadPicture("plugins/signalgen/freqbut.png");

	px1 = app->LoadPicture("images/1px.png");

	popupX = app->GetWindowWidth() / 2 - popupWidth / 2;
	popupY = app->GetWindowHeight() / 2 - popupHeight / 2;

	button[0] = app->LoadPicture("plugins/signalgen/gen.png");
	button[1] = app->LoadPicture("plugins/signalgen/gen_over.png");

	close[0] = app->LoadPicture("plugins/mymfx/close.png");
	close[1] = app->LoadPicture("plugins/mymfx/close_over.png");

}

void SignalGenMFX::DrawEffect()
{
	app->DrawPicture(back, popupX, popupY);

	app->DrawString("Sine Wave Frequency in Hz:", popupX, popupY + 50, popupX + popupWidth, popupY + 70);

	for (int i = 0; i < maxfrequencies; ++i)
	{
		float x, y, w, h;
		x = popupX + 20 + i * 60;
		y = popupY + 80;
		w = 50;
		h = 20;

		if (app->MouseInBox(x, y, w, h))
		{
			if (app->MouseClick())
				curfreq = i;
		}

		if (curfreq == i)
		{
			app->SetColor(100, 255, 100, 255);
		}
		else
		{
			app->SetColor(255, 255, 255, 255);
		}
		app->DrawPicture(freqbut, x, y, w, h);
		
		/*if (curfreq == i)
		{
			app->SetColor(200, 200, 200, 255);
		}
		else
		{
			app->SetColor(0, 0, 0, 255);
		}*/
		app->SetColor(0, 0, 0, 255);

		char buf[256];
		sprintf(buf, "%i", (int)frequencies[i]);
		app->DrawString(buf, x, y, x+w, y+h);
	}

	app->ResetColor();

	if (app->DrawButton(button, popupX + 135, popupY + 120))
	{
		app->ApplyEffect(this, false);
		app->CloseEffect();
	}

	if (app->KeyTrigged(DIK_ESCAPE))
	{
		app->CloseEffect();
	}

	if (app->DrawButton(close, popupX + 360, popupY + 8))
	{
		app->CloseEffect();
	}

	if (app->MouseClick() && !app->MouseInBox(popupX, popupY, popupWidth, popupHeight))
	{
		app->CloseEffect();
	}
}

#define REC_PI 3.14159265358979323846

int SignalGenMFX::ProcessEffect(float* buffer, int length, float mix)
{
	//length = 48000 * 10;
	for (int i = 0; i < length; ++i)
	{
		buffer[i] = sinf(2.0f * REC_PI * frequencies[curfreq] * (float)i / 48000.0f) * mix;
	}
	return length;
}

const char* SignalGenMFX::GetEffectName()
{
	return "Signal Generator";
}

extern "C" MFXAPI MorningFX* GetMyMFX()
{
	return &myeffect;
}
