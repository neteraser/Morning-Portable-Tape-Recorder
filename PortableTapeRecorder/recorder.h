#pragma once

//#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <math.h>
#include <cmath>
#include "bass/bass.h"
#include "audiolibwrapper.h"

#define REC_PI 3.14159265358979323846

#define RECBUFFERDELAYMS 38
//#define RECBUFFERLONGDELAYMS 1500

#define RECUSELIBAUDIOFILE 1
#define RECUSELIBLAME 1

#define RECSAMPLERATE 48000
#define RECBUFSAMPLES(seconds) (seconds * RECSAMPLERATE)
#define RECBUFSTANDARDSIZE RECBUFSAMPLES(250)
//#define RECBUFSIZEWITHZEROCRASHES RECBUFSAMPLES(260)
#define SAFEBUFRESERVESIZE RECBUFSAMPLES(10)

#define RECMAXUNDOLEVEL (12)

// cheap ADC: 257 (256+1)
// expensive ADC: 266 (33+1)
#define RECCHEAPADCBUF (257)
#define RECEXPENSIVEADCBUF (266)
#define RECSLOWDOWNSAMPLES (258)

#define RECBEEPLENMAX (8192)

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

__inline float Int16ToFloat(short in)
{
	float out = 0;
	if (in > 0)
		out = ((float)in) / 32767.0f;
	else
	if( in < 0)
		out = ((float)in) / 32768.0f;
	return out;
}

__inline short FloatToInt16(float in)
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
	if(in < 0.0f)
	{
		float v = in * 32768.0;
		out = (short)truncf(v);
		if ( fabs(v - (float)out) > 0.6f)
			out -= 1;
	}
	return out;
}

__inline short PackInt16ForWriting(short in)
{
	short out;
	char bv[2];
	bv[0] = (char)((in) >> 8);
	bv[1] = (char)( ((in) << 8) >> 8);

	char signbit;
	signbit = ( bv[0] & 0b10000000 ) >> 7;
	bv[0] = bv[0] << 1;
	bv[0] = bv[0] | signbit;

	out = *(short*)bv;
	return out;
}

__inline short UnpackInt16ForReading(short in)
{
	short out;
	char bv[2];
	bv[0] = (char)((in) >> 8);
	bv[1] = (char)(((in) << 8) >> 8);

	char signbit;
	signbit = bv[1] & 0b00000001;
	signbit = signbit << 7;
	bv[1] = bv[1] >> 1;
	bv[1] = bv[1] | signbit;

	out = *((short*)bv);

	return out;
}
	/*char bswap;
	bswap = bv[0];
	bv[0] = bv[1];
	bv[1] = bswap;*/

__inline float PolarizeFloat(float in, bool plusorminus = true /*plus*/, bool writeorread = true /*write*/)
{
//return in;
//	return in;
	float out = in;
	// clamp here, let it be
	if (out > 1.0f)
		out = 1.0f;
	if (out < -1.0f)
		out = -1.0f;

	if (plusorminus)
	{
		if(out < 0.0)
		{
			if (writeorread)
			{
				out -= 1.0f / 32768.0f;
				if (out < -1.0)
					out = -1.0;
			}

			out = -(1.0f - fabs(out));

			if (!writeorread)
			{
				out += 1.0f / 32768.0f;
				if (out > 0.0)
					out = 0.0;
			}
		}
	}
	return out;
}

__inline float ExorciseFloatNoise(float in)
{
	float out = in * 0.9999991666666f;
	return out;
}

__inline float CatmullRom1D(float t, float p0, float p1, float p2, float p3)
{
	float t2 = t * t;
	float t3 = t2 * t;
	return (0.5f * ((2.0f * p1) + (-p0 + p2) * t +
		(2.0f * p0 - 5.0f * p1 + 4 * p2 - p3) * t2 +
		(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3));
}

void InitFolders();
void InitRec(Record& rec);
void LiftRec();
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

void PreprocessRec(Record& rec, int preprocesslen);

void PostProcessRec(Record& rec, bool dofadeout = true);

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

__inline float VolumeToDb(float volume)
{
	if (fabs(volume) < 0.00001f)
		return -100.0f;
	return 20.0f * log10f(fabs(volume));
}


void SaveRec(bool wavormp3 = true);

void WriteRec(bool forcerec = false);

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

//extern DWORD gerror;

bool RecorderDeviceRetrieve(int devNum = -1, int recDevNum = -1);

void RecorderSetLoop(bool loop);

bool RecorderIsLooping();

void RecorderSetRewriteMode(bool hddrewrite);

void RecorderSetQFX(float qfxvalue = 1.0f /* 0.1...1.0f */);

float RecorderGetQFX();

void RecorderLoadBeepSound(const float* buf, int len /* len to 8192 max */);

void RecorderFixRemasterOffset();

float RecCalculatePeak(Record& rec, int begin = 0, int end = -1);

void RecorderSetColoration(int clr /* 0, 1, 2 supports three coloration */);

bool RecorderPushData(const float* buf, int samples);

bool RecorderPullData(float* buf, int samples);

void RecorderSetCallbacks();

void RecorderSetStretchSamples(int stretchSamplesNumber);

void RecorderReRender();

void RecorderAddReverb();

void RecorderHighCut(Record& rec);

void RecorderHighBoost(Record& rec);

void RecorderSuperfi();

int RecorderGetLastSamplesNumber();

int RecorderGetTrollOrJoushState();

void RecorderSetRegion(int region_begin, int region_end);

void RecorderGetRegion(int& begin, int& end);

bool IsRecorderRegionSet();

void RecorderResetRegion();

void RecorderCutToRegion();

void RecorderCutOutRegion();

void RecorderMuteRegion();

class AutoNote
{
public:
	int pos;
	float energy;

	bool const operator < (const AutoNote& n1)
	{
		return pos < n1.pos;
	}
};

void RecorderApplyAutoNote();

extern std::vector<AutoNote> recpeaks;