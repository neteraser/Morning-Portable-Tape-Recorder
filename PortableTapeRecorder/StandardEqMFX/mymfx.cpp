
#include "mymfx.h"
#include <dinput.h>

MyMFX myeffect;

int back;
int button[2];
int close[2];

int popupX = 0, popupY = 0;
int popupWidth = 200, popupHeight = 200;

void MyMFX::LoadEffect()
{
	back = app->LoadPicture("plugins/mymfx/back.png");
	popupX = app->GetWindowWidth() / 2 - popupWidth / 2;
	popupY = app->GetWindowHeight() /2 - popupHeight / 2;

	button[0] = app->LoadPicture("plugins/mymfx/apply.png");
	button[1] = app->LoadPicture("plugins/mymfx/apply_over.png");

	close[0] = app->LoadPicture("plugins/mymfx/close.png");
	close[1] = app->LoadPicture("plugins/mymfx/close_over.png");

}

void MyMFX::DrawEffect()
{
	app->DrawPicture(back, popupX, popupY);

	app->DrawString("MyMFX Booster", popupX, popupY + 165, popupX + popupWidth, popupY + 185);

	if (app->DrawButton(button, popupX + 35, popupY + 90))
	{
		app->ApplyEffect(this, false);
	}

	if (app->KeyTrigged(DIK_ESCAPE))
	{
		app->CloseEffect();
	}

	if (app->DrawButton(close, popupX+160, popupY+8))
	{
		app->CloseEffect();
	}

	if (app->MouseClick() && !app->MouseInBox(popupX, popupY, popupWidth, popupHeight))
	{
		app->CloseEffect();
	}
}

int MyMFX::ProcessEffect(float* buffer, int length, float mix)
{
	for (int i = 0; i < length; ++i)
	{
		buffer[i] *= 1.66f;
	}
	return length;
}

const char* MyMFX::GetEffectName()
{
	return "x1.66 Booster";
}

extern "C" MFXAPI MorningFX* GetMyMFX()
{
	return &myeffect;
}