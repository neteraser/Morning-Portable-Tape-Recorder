
#include "parametriceq.h"
#include <dinput.h>

ParametricEQMFX myeffect;

int back;
int line;
int button[2];
int apply[2];
int close[2];
int align;
int tnb[2];
int lpf[2];
int inv[2];

int popupX = 0, popupY = 0;
int popupWidth = 850, popupHeight = 350;

#define EQBANDS (30)

float frequencies[EQBANDS] = { 0.0f };

float bandpos[EQBANDS] = { 0.5f };

void ParametricEQMFX::LoadEffect()
{
	back = app->LoadPicture("plugins/parametriceq/back.png");

	line = app->LoadPicture("plugins/parametriceq/line.png");
	 
	popupX = app->GetWindowWidth() / 2 - popupWidth / 2;
	popupY = app->GetWindowHeight() / 2 - popupHeight / 2 - 6;

	button[0] = app->LoadPicture("plugins/parametriceq/button.png");
	button[1] = app->LoadPicture("plugins/parametriceq/button_over.png");

	apply[0] = app->LoadPicture("plugins/parametriceq/equalize.png");
	apply[1] = app->LoadPicture("plugins/parametriceq/equalize_over.png");

	align = app->LoadPicture("plugins/parametriceq/align.png");

	tnb[0] = app->LoadPicture("plugins/parametriceq/tnb.png");
	tnb[1] = app->LoadPicture("plugins/parametriceq/tnb_over.png");

	lpf[0] = app->LoadPicture("plugins/parametriceq/lpf.png");
	lpf[1] = app->LoadPicture("plugins/parametriceq/lpf_over.png");

	inv[0] = app->LoadPicture("plugins/parametriceq/invert.png");
	inv[1] = app->LoadPicture("plugins/parametriceq/invert_over.png");

	close[0] = app->LoadPicture("plugins/mymfx/close.png");
	close[1] = app->LoadPicture("plugins/mymfx/close_over.png");

	frequencies[0] = 600.0f;

	for (int i = 1; i < EQBANDS; ++i)
	{
		frequencies[i] = frequencies[i-1] + 600.0f;
	}

	for (int i = 0; i < EQBANDS; ++i)
		bandpos[i] = 0.0f;

}

void ParametricEQMFX::DrawEffect()
{
	app->DrawPicture(back, popupX, popupY);

	//app->DrawString("MyMFX Booster", popupX, popupY + 165, popupX + popupWidth, popupY + 185);

	static int movingband = -1;

	static int bandover = -1;

	if (movingband >= 0)
	{
		bandpos[movingband] += (float)app->MouseMoveY() / 250.0f;
		if (bandpos[movingband] < 0.0)
			bandpos[movingband] = 0.0f;
		if (bandpos[movingband] > 1.0f)
			bandpos[movingband] = 1.0f;

		if (!app->MouseL())
			movingband = -1;
	}

	bool nobandisover = true;

	static DWORD lightupTime = 0;

	for (int i = 0; i < EQBANDS; ++i)
	{
		float x = popupX + i * 25 + 25 + 8;
		float y = popupY + 50;
		float y2 = y + 15 * (i % 2);

		float y3 = 168.0f;

		app->ResetColor();
		if(movingband == i || bandover == i || (timeGetTime() - lightupTime < 600))
			app->SetColor(255 * bandpos[i], 255 * (1.0f - bandpos[i]), (1.0f - fabs(bandpos[i] * 2.0f - 1.0f)) * 255.0f, 255);
		app->DrawPicture(line, x, y);
		app->ResetColor();

		if (app->MouseInBox(x - 8, y + bandpos[i] * y3, 22, 17) )
		{
			bandover = i;
			nobandisover = false;

			app->DrawPicture(button[1], x - 8, y + bandpos[i] * y3);

			if (app->MouseL() && movingband < 0)
			{
				movingband = i;
			}
		}
		else
		{
			app->DrawPicture(button[ movingband == i ? 1 : 0 ], x - 8, y + bandpos[i] * y3);

			if (app->MouseInBox(x - 5, y, 20, 185))
			{
				nobandisover = false;
 				bandover = i;
				if(app->MouseClick())
				{
					bandpos[i] = (app->MouseY() - y) / 185.0f;
				}
			}
		}

		char buf[64];
		float freq = frequencies[i];

		sprintf_s(buf, 64, "%.1f", (frequencies[i] / 1000.0f));

		app->DrawString(buf, x - 10.0f, y2 + 185, x + 15.0f, y2 + 205);
	}

	if(nobandisover)
		bandover = -1;

	static int currentalign = 0;

	{
		float x = popupX + EQBANDS * 25 + 40;
		float y = popupY + 50;
		float y2 = 165.0f * 0.5f;
		bool over = false;

		if (app->MouseInBox(x - 8, y - 2, 55, 25))
		{
			over = true;
			if (app->MouseClick())
			{
				for (int i = 0; i < EQBANDS; ++i)
				{
					bandpos[i] = 0.0f;
				}
				currentalign = 0;
			}
		}
		else
			over = false;

		app->SetColor(255, 255, 255, 100);

		if (currentalign == 0 || over)
			app->ResetColor();
			
		app->DrawPicture(align, x-8, y - 2);
		app->DrawString("0dB", x + 3, y, x + 45, y + 20);

		if (app->MouseInBox(x - 8, y + y2 - 2, 55, 25))
		{
			if (app->MouseClick())
			{
				currentalign = 1;
				for (int i = 0; i < EQBANDS; ++i)
				{
					bandpos[i] = 0.5f;
				}
			}
			over = true;
		}
		else
			over = false;

		app->SetColor(255, 255, 255, 100);

		if (currentalign == 1 || over)
			app->ResetColor();
			
		app->DrawPicture(align, x - 8, y + y2 - 2);
		app->DrawString("-6dB", x + 3, y + y2, x + 45, y + y2 + 20);

		if (app->MouseInBox(x - 8, y + y2 * 2 - 2, 55, 25))
		{
			if (app->MouseClick())
			{
				currentalign = 2;
				for (int i = 0; i < EQBANDS; ++i)
				{
					bandpos[i] = 1.0f;
				}
			}
			over = true;
		}
		else
			over = false;

		app->SetColor(255, 255, 255, 100);

		if (currentalign == 2 || over)
			app->ResetColor();

		app->DrawPicture(align, x - 8, y + y2 * 2 - 2);
		app->DrawString("-16dB", x + 3, y + y2 * 2, x + 45, y + y2 * 2 + 20);

		app->ResetColor();
	}

	if (app->DrawButton(tnb, popupX + 160, popupY + 285))
	{
		for (int i = 0; i < EQBANDS; ++i)
			bandpos[i] = 0.5f;

		bandpos[0] -= 0.5f;
		bandpos[1] -= 0.45f;
		bandpos[2] -= 0.33f;
		bandpos[3] -= 0.22f;
		bandpos[4] -= 0.1f;

		bandpos[29] += 0.47f;
		bandpos[28] += 0.33f;
		bandpos[27] += 0.22f;
		bandpos[26] += 0.1f;
		bandpos[25] += 0.047f;

		lightupTime = timeGetTime();
	}

	if (app->DrawButton(lpf, popupX + 280, popupY + 285))
	{
		for (int i = 0; i < EQBANDS; ++i)
		{
			bandpos[i] = ((float)i / (float)EQBANDS) * 0.5f;
		}
		lightupTime = timeGetTime();
	}

	if (app->DrawButton(inv, popupX + 400, popupY + 285))
	{
		for (int i = 0; i < EQBANDS; ++i)
			bandpos[i] = 1.0f - bandpos[i];
		lightupTime = timeGetTime();
	}


	if (app->DrawButton(apply, popupX + 700, popupY + 300) || app->KeyTrigged(DIK_RETURN))
	{
		app->ApplyEffect(this, true);
		app->CloseEffect();
	}

	if (app->KeyTrigged(DIK_ESCAPE))
	{
		app->CloseEffect();
	}

	if (app->DrawButton(close, popupX + 810, popupY + 8))
	{
		app->CloseEffect();
	}

	if (app->MouseClick() && !app->MouseInBox(popupX, popupY, popupWidth, popupHeight))
	{
		app->CloseEffect();
	}
}

#ifdef REC64BIT

#include <mkl.h>

#define FFTSIZE (8192)

#define BUFSIZE (FFTSIZE * 2)

#define FFTOVERLAPPING 8

float rbuf[BUFSIZE];
float ibuf[BUFSIZE];

void mkl_fft(float* buf)
{
	CopyMemory(rbuf, buf, FFTSIZE * sizeof(float));
	ZeroMemory(ibuf, FFTSIZE * sizeof(float));

	DFTI_DESCRIPTOR_HANDLE desc;
	DftiCreateDescriptor(&desc, DFTI_SINGLE, DFTI_COMPLEX, 1, FFTSIZE);
	DftiSetValue(desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE);
	DftiCommitDescriptor(desc);
	DftiComputeForward(desc, rbuf, ibuf);
	DftiFreeDescriptor(&desc);
}

void mkl_ifft(float* buf)
{
	DFTI_DESCRIPTOR_HANDLE desc;
	DftiCreateDescriptor(&desc, DFTI_SINGLE, DFTI_COMPLEX, 1, FFTSIZE); 
	DftiSetValue(desc, DFTI_PLACEMENT, DFTI_NOT_INPLACE); //Out of place FFT
	//DftiSetValue(desc, DFTI_BACKWARD_SCALE, 1.0f / FFTSIZE); //Scale down the output
	DftiCommitDescriptor(desc); 
	DftiComputeBackward(desc, rbuf, ibuf); 
	DftiFreeDescriptor(&desc); 

	CopyMemory(buf, rbuf, FFTSIZE * sizeof(float));
}

void mkl_mult(int i, float mul)
{
	rbuf[i] *= mul;
	ibuf[i] *= mul;
}

#define eq_fft mkl_fft
#define eq_ifft mkl_ifft
#define eq_mult mkl_mult

#else

#include "KissFFT/kiss_fftr.h"

#define FFTSIZE (4096)

#define FFTOVERLAPPING 16

#define BUFSIZE (FFTSIZE * 2)

kiss_fft_cpx cpxbuf[BUFSIZE];

void kiss_fft(float* buf)
{
	kiss_fftr_cfg cfg = kiss_fftr_alloc(FFTSIZE, 0, 0, 0);

	kiss_fftr(cfg, buf, cpxbuf);

	kiss_fftr_free(cfg);
}

void kiss_ifft(float* buf)
{

	kiss_fftr_cfg cfg = kiss_fftr_alloc(FFTSIZE, 1, 0, 0);

	kiss_fftri(cfg, cpxbuf, buf);

	kiss_fftr_free(cfg);

	for (int i = 0; i < FFTSIZE; ++i)
	{
		buf[i] /= (float)FFTSIZE;
	}

}

void kiss_mult(int i, float mul)
{
	cpxbuf[i].r *= mul;
	cpxbuf[i].i *= mul;
}

#define eq_fft kiss_fft
#define eq_ifft kiss_ifft
#define eq_mult kiss_mult

#endif

int ParametricEQMFX::ProcessEffect(float* buffer, int length, float mix)
{
//	for (int i = 0; i < length; ++i)
//	{
//		buffer[i] *= 1.66f;
//	}
	float* newbuffer = (float*)malloc(sizeof(float) * ( length + FFTSIZE * 4));

	ZeroMemory(newbuffer, sizeof(float) * length);

	float tempbuf[FFTSIZE];
	float overlapbuf[FFTOVERLAPPING];
	bool firstoverlap = true;

	ZeroMemory(tempbuf, sizeof(float) * FFTSIZE);

	for (int i = 0; i < length; i += FFTSIZE - FFTOVERLAPPING)
	{
		for (int j = 0; j < EQBANDS; ++j)
		{

			eq_fft(buffer + i);

			float band_size = (float)FFTSIZE / (float)EQBANDS;
			int band_begin = truncf(j * band_size);
			int band_end = truncf((j + 1) * band_size);
			for (int k = band_begin; k < band_end; ++k)
			{
				eq_mult(k, 0.25f);
				//rbuf[k] *= 0.25f;//(1.0f - bandpos[j]);// *0.5f;
				//ibuf[k] *= 0.25f;// (1.0f - bandpos[j]);// *0.5f;
			}

			eq_ifft(tempbuf);

			for (int k = 0; k < FFTSIZE; ++k)
				newbuffer[i+k] += tempbuf[k] * (1.0f - bandpos[j]);
		}

		for (int k = 0; k < FFTSIZE; ++k)
			newbuffer[i + k] = buffer[i+k] * (1.0f - mix) + newbuffer[i+k] * mix / (float)EQBANDS;

		if (!firstoverlap)
		{
			for (int k = 0; k < FFTOVERLAPPING; ++k)
			{
				int ki = i + FFTSIZE - FFTOVERLAPPING + k;
				newbuffer[ki] = (newbuffer[ki] + overlapbuf[k]) * 0.5f;
			}
		}
		else
			firstoverlap = false;

		for (int k = 0; k < FFTOVERLAPPING; ++k)
			overlapbuf[k] = newbuffer[i + FFTSIZE - FFTOVERLAPPING + k];


		//float prev1, prev2;
		//prev1 = newbuffer[i];
		//prev2 = newbuffer[i + 1];

		app->ReportEffectProgress((float)i / (float)length);
	}

	CopyMemory(buffer, newbuffer, sizeof(float) * length);

	free(newbuffer);

	return length;
}

const char* ParametricEQMFX::GetEffectName()
{
	return "Parametric EQ";
}

extern "C" MFXAPI MorningFX* GetMyMFX()
{
	return &myeffect;
}