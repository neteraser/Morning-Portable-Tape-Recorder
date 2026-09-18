#pragma once

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include "bass/bass.h"

#define RECSAMPLERATE 48000
#define RECBUFSAMPLES(seconds) (seconds * RECSAMPLERATE)
#define RECBUFSTANDARDSIZE RECBUFSAMPLES(250)
//#define RECBUFSIZEWITHZEROCRASHES RECBUFSAMPLES(260)
#define SAFEBUFRESERVESIZE RECBUFSAMPLES(10)

#define RECMAXUNDOLEVEL (10)

// cheap ADC: 257 (256+1)
// expensive ADC: 266 (33+1)
#define RECCHEAPADCBUF (257)
#define RECEXPENSIVEADCBUF (266)
#define RECSLOWDOWNSAMPLES (258)

struct Record
{
	HRECORD record;
	HSTREAM stream;
	FILE* tape;
	FILE* tapepolarized;
	float safebuf_before[SAFEBUFRESERVESIZE];
	float recbuf[RECBUFSTANDARDSIZE];
	float safebuf_after[SAFEBUFRESERVESIZE];
	float rectempbuf[16384];
	int temppos;
	int bufpos;
	int playpos;
	int writepos;
	int unipos;
	BOOL breakrec;
	BOOL breakplay;
	float peak;
	int peakpos;
	unsigned lastreccallback_time;
	unsigned lastplaycallback_time;
	unsigned playcallback_dt;
};

extern Record baserec;
extern Record streamrec;
 
/*extern BOOL CALLBACK record_proc(HRECORD handle,
	const void* buffer,
	DWORD length,
	void* user);
	*/

inline float Int16ToFloat(short in)
{
	float out = 0;
	if (in > 0)
		out = (float)(in) / 32767.0f;
	else
		out = (float(in)) / 32768.0f;
	return out;
}

inline short FloatToInt16(float in)
{
	// clamp for saving
	if (in > 1.0f)
		in = 1.0f;
	if (in < -1.0f)
		in = -1.0f;

	short out = 0;
	if (in > 0.0)
	{
		float v = in * 32767.0f;
		// precision bit -- very important;
		out = (short)truncf(v);
		if (v - (float)out > 0.6f)
			out += 1;
	}
	else
	{
		float v = in * 32768.0;
		out = (short)truncf(v);
		if ( v - (float)out < -0.6f)
			out -= 1;
	}
	return out;
}

__inline short PackInt16ForWriting(short in)
{
	short out;
	char bv[2];
	bv[0] = (char)((short)in >> 8);
	bv[1] = (char)(((short)in << 8) >> 8);

	char signbit;
	signbit = bv[1] >> 7;
	bv[1] = bv[1] << 1;
	bv[1] = bv[1] | signbit;

	out = *((short*)bv);
	return out;
}

__inline short UnpackInt16ForReading(short in)
{
	short out;
	char bv[2];
	bv[0] = (char)((short)in >> 8);
	bv[1] = (char)(((short)in << 8) >> 8);

	char signbit;
	signbit = bv[0] & 0b00000001;
	signbit = signbit << 7;
	bv[0] = bv[0] >> 1;
	bv[0] = bv[0] | signbit;

	out = *((short*)bv);

	return out;
}

__inline float PolarizeFloat(float in, bool plusorminus = true /*plus*/, bool writeorread = true /*write*/)
{
	float out = in;
	// clamp here, let it be
	if (out > 1.0f)
		out = 1.0f;
	if (out < -1.0f)
		out = -1.0f;

	if (plusorminus)
	{
		if (in > 0)
		{
			out = 1.0f - in;
		}
	}
	else
	{
		if (in < 0)
		{
			out = -(1.0f - fabs(in));
		}
	}
	return out;
}

float CatmullRom1D(float t, float p0, float p1, float p2, float p3);

void InitFolders();
void InitRec(Record& rec);
void StartRec(Record& rec);

struct RIAAFilter {
	double b0, b1, b2, a1, a2;
	double z1_state, z2_state;
	double x2, x1, y2, y1;
	double t1, t2, t3;
};

extern RIAAFilter baseriaafilter;
extern RIAAFilter baseinvriaafilter;

void PrepareRIAAFilter(RIAAFilter& filter, double t1 = 30, double t2 = 150, double t3 = 2000);
double ProcessRIAAFilter(RIAAFilter& filter, double in);

void PrepareInverseRIAAFilter(RIAAFilter& filter, double t1 = 30, double t2 = 150, double t3 = 2000);
double ProcessInverseRIAAFilter(RIAAFilter& filter, double in);

void RecProcess256Samples();

void PreprocessRec(Record& rec);

void PostProcessRec(Record& rec);

void NormalizeRec(Record& rec);

void LimitRec(Record& rec);

void AddRIAAToRec(Record& rec);

void UndoFXFromRec();

void ReadRec();

void StopRec(Record& rec);

void PlayRec();

void StopPlayingRec();

extern DWORD CALLBACK play_proc(HSTREAM handle,
	void* buffer,
	DWORD length,
	void* user);

float ComputeAverage(Record& rec, int from, int to, int step = 1);

void ComputeWaveform(Record& rec, int from, int to, int step, float& outplus, float& outminus);

float ComputeRMS(Record& rec, int from, int to, int step = 1);

inline float VolumeToDb(float volume)
{
	if (fabs(volume) < 0.00001f)
		return -100.0f;
	return 20.0f * log10f(fabs(volume));
}

void SaveRec(bool wavormp3 = true);

void WriteRec();

void RecorderDebugCheck();

void ResetDummySinus();

float GetDummySinus();

void RecorderUpdate(float dt);

extern int recOrPlay;

extern int recOrStop;

extern int playOrStop;

extern int undolevel;

extern int remastermode;


bool CheckRecorderPeak(float newvalue, int bufpos = -1);

void StartRemaster();

void StopRemaster();

bool IsRecorderPlaying();

bool IsRecorderWriting();

bool IsRecorderIdling();

bool IsRecorderRemastering();

bool RecorderCanUndo();

bool RecorderCanDoFX();

extern HWND mainWnd;

extern DWORD error;

bool RecorderDeviceRetrieve(int devNum = -1, int recDevNum = -1);

void RecorderSetLoop(bool loop);