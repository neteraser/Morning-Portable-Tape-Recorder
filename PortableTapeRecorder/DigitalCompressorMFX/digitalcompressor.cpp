
#include "digitalcompressor.h"
#include <dinput.h>
#include <vector>

DigitalDeadMeterMFX mymeter;
DigitalCompressorMFX myeffect;

int px1;
int back;
int pack[2];
int button[2];
int close[2];

int popupX = 0, popupY = 0;
int popupWidth = 500, popupHeight = 250;

double rms = 0.0f;
float recMax = 0.0f;
bool computeRMS = false;
bool doCompress = false;

__inline float VolumeToDb(float volume)
{
	if (fabs(volume) < 0.00001f)
		return -100.0f;
	return 20.0f * log10f(fabs(volume));
}

void DigitalCompressorMFX::LoadEffect()
{
	px1 = app->LoadPicture("images/1px.png");

	back = app->LoadPicture("plugins/digitalcompressor/back.png");
	popupX = app->GetWindowWidth() / 2 - popupWidth / 2;
	popupY = app->GetWindowHeight() / 2 - popupHeight / 2;

	pack[0] = app->LoadPicture("plugins/digitalcompressor/pack.png");
	pack[1] = app->LoadPicture("plugins/digitalcompressor/pack_over.png");

	close[0] = app->LoadPicture("plugins/mymfx/close.png");
	close[1] = app->LoadPicture("plugins/mymfx/close_over.png");

	computeRMS = true;
}

void DigitalCompressorMFX::DrawEffect()
{
	app->DrawPicture(back, popupX, popupY, popupWidth, popupHeight);

	float rmsdb = VolumeToDb(rms);
	if (rmsdb < -16.0f)
		rmsdb = -16.0f;
	if (rmsdb > -6.0f)
		rmsdb = -6.0f;
	float rmsbar = 1.0f - (fabs(rmsdb) - 6.0f) / 10.0f;
	if (rmsbar < 0.01f)
		rmsbar = 0.01f;
	if (rmsbar > 1.0f)
		rmsbar = 1.0f;

	app->SetColor(255.0f * ( 1.0f - rmsbar), 255.0f * rmsbar, 0, 255);
	
	app->DrawPicture(px1, popupX + 60.0f, popupY + 118.0f, 380.0f * rmsbar, 24.0f);

	app->SetColor(255, 255, 255, 255);
	
	double rmstitle = VolumeToDb(rms);
	char buf[256];
	if (rmstitle < -16.0f)
	{
		sprintf_s(buf, 256, "RMS: < -16db", (float)rmsdb);
	}
	else
	if (rmstitle > -6.0f)
	{
		sprintf_s(buf, 256, "RMS: > -6db", (float)rmsdb);
	}
	else
	{
		sprintf_s(buf, 256, "RMS: %.2f db", (float)rmsdb);
	}
	
	app->DrawString(buf, popupX + 60.0f, popupY + 120.0f, popupX + 440.0f, popupY + 140.0f);

	app->ResetColor();

	bool cancompress = true;
	if (rmsbar >= 1.0f)
	{
		app->SetColor(255, 255, 255, 128);
		cancompress = false;
	}

	if (app->DrawButton(pack, popupX + 165, popupY + 180) && cancompress)
	{
		doCompress = true;
		app->ApplyEffect(this, false);
	}

	//app->DrawPicture(back, popupX, popupY);

	//app->DrawString("MyMFX Booster", popupX, popupY + 165, popupX + popupWidth, popupY + 185);

	//if (app->DrawButton(button, popupX + 35, popupY + 90))
	//{
	//	app->ApplyEffect(this, false);
	//}

	if (app->KeyTrigged(DIK_ESCAPE))
	{
		app->CloseEffect();
	}

	if (app->DrawButton(close, popupX + 460, popupY + 8))
	{
		app->CloseEffect();
	}

	if (app->MouseClick() && !app->MouseInBox(popupX, popupY, popupWidth, popupHeight))
	{
		app->CloseEffect();
	}

	if (computeRMS)
	{
		app->ApplyEffect(&mymeter, false);
	}
}

int DigitalDeadMeterMFX::ProcessEffect(float* buffer, int length, float mix)
{
	if (computeRMS)
	{
		double vsum = 0.0f;
		recMax = 0.0f;
		for (int i = 0; i < length; ++i)
		{
			float fv = fabs(buffer[i]);
			if (fv > recMax)
				recMax = fv;
		}

		for (int i = 0; i < length; ++i)
		{
			float fv = fabs(buffer[i]);
			fv *= (1.0f / recMax);
			vsum += fv * fv;
		}
		if (length != 0)
		{
			vsum /= (double)length;
			rms = sqrtf(vsum);
		}
		else
		{
			rms = 0.0f;
		}
		//computeRMS = false;
	}

	return 0;
}

struct compressionpeak_t
{
	int begin;
	int end;
};

std::vector<compressionpeak_t> peaks;

int DigitalCompressorMFX::ProcessEffect(float* buffer, int length, float mix)
{
	if(doCompress)
	{
		peaks.clear();
		bool ispeak = false;
		compressionpeak_t peak;
		for (int i = 0; i < length; ++i)
		{
			float fv = fabs(buffer[i]);
			fv *= (1.0f / recMax);

			if (fv > 0.66f)
			{
				if (!ispeak)
				{
					peak.begin = i;
				}
				ispeak = true;
			}
			else
			{
				if (ispeak)
				{
					if (peak.begin < i) // assert
					{
						peak.end = i;
						peaks.push_back(peak);
					}
				}
				ispeak = false;
			}
		}

		for (int i = 0; i < peaks.size(); ++i)
		{
			peak = peaks[i];
			for (int j = peak.begin; j < peak.end; ++j)
			{
				int len = peak.end - peak.begin;
				float scale = (float)(peak.end - j) / (float)len;
				float scale2 = 1.0f - fabs(scale * 2.0f - 1.0f);
				float fv = buffer[j] * (1.0f / recMax);
				float limpeak = fabs(fv) - 0.66f;
				//buffer[j] = scale2 * 0.5f;
				if (fv > 0.0f)
				{
					buffer[j] = (0.66f + limpeak * scale2 * 0.5f ) * recMax;
				}
				else
				{
					buffer[j] = (- 0.66f - limpeak * scale2 * 0.5f) * recMax;
				}
			}
		}

		doCompress = false;
	}
	return length;
}

const char* DigitalCompressorMFX::GetEffectName()
{
	return "Digital Compressor";
}

extern "C" MFXAPI MorningFX* GetMyMFX()
{
	return &myeffect;
}