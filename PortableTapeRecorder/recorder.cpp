
#include "recorder.h"
#include "log.h"
#include <string>
#include <cmath>
#include <fstream>
#ifdef RECUSELIBAUDIOFILE
#include "audiofile.h"
#endif
#ifdef RECUSELIBLAME
#include "lame/lame.h"
#endif
#include <immintrin.h>
#include <direct.h>
#include "morningreverb.h"

#include "MorningHiddenAudioProcessing/MorningHidden.h"

Record baserec;
Record streamrec;

float recbeep[RECBEEPLENMAX];
int recbeep_initialoffset = 0;
float recbeepregion[96000];

//int recOrStop = 1;
//int playOrStop = 1;

int stretchSamples = 2;
extern int adcIntMaxNatural;

std::string gfilename;

bool bDoRewrites = true;

float recQFX = 1.0f;

void InitFolders()
{
	gfilename.reserve(1024);
	gfilename = getenv("PROGRAMDATA");
	gfilename += "\\Morning\\";
	_mkdir(gfilename.c_str());
	gfilename += "\\PortableTapeRecorder\\";
	_mkdir(gfilename.c_str());

	gfilename = getenv("APPDATA");
	gfilename += "\\PortableTapeRecorder\\";
	_mkdir(gfilename.c_str());

	gfilename = getenv("USERPROFILE");
	gfilename += "\\Music\\";
	if (GetFileAttributes(gfilename.c_str()) == INVALID_FILE_ATTRIBUTES)
	{
		_mkdir(gfilename.c_str());
	}
	//_mkdir(gfilename.c_str());
	gfilename += "\\PortableTapeRecorder\\";
	_mkdir(gfilename.c_str());
	gfilename += "\\output\\";
	_mkdir(gfilename.c_str());
}

const char* getProgramFileName(const char* filename)
{
	gfilename = getenv("PROGRAMDATA");
	gfilename += "\\Morning\\PortableTapeRecorder\\";
	gfilename += filename;
	return gfilename.c_str();
}

const char* getAppFileName(const char* filename)
{
	gfilename = getenv("APPDATA");
	gfilename += "\\PortableTapeRecorder\\";
	gfilename += filename;
	return gfilename.c_str();
}

const char* getMusicFileName(const char* filename)
{
	gfilename = getenv("USERPROFILE");
	gfilename += "\\Music\\PortableTapeRecorder\\";
	gfilename += filename;
	return gfilename.c_str();
}


extern std::string exportartist;

bool playloop = false;

int recOrPlay= 0;

bool noRecDevice = false;

float recDeviceVolume = 0.0f;
float recDeviceRecTime = 0.0;

float masterDevicePlayTime = 0.0f;

float recFadeInTime = 2.5f;

static int recCLRFX = 0;

__inline float FixDeviceBit(float in)
{
	float out = -in;
	out *= (float)(adcIntMaxNatural) / 32767.0f;
	if (out < 0.0f)
	{
		out *= 32768.0f / 32767.0f;
	}
	BEGINHIDDENCODE(HiddenProcessSampleByColor)
		out = HiddenProcessSampleByColor(out, recQFX, recCLRFX);
	ENDHIDDENCODE
	return -out;
}

bool RecorderPushData(const float* buffer, int length)
{
	_m_prefetchrs(baserec.recbuf + baserec.bufpos);
	_m_prefetchrs(buffer);

	if(remastermode != 0)
		recOrPlay = -1;

	Record* prec = &baserec;//(Record*)user;
	float* fbuf = (float*)buffer;

	if (prec->breakrec == TRUE)
	{
		return false;
	}

//	if (timeGetTime() - prec->lastreccallback_time > 1000)
//		return FALSE;

	prec->lastreccallback_time = timeGetTime();

	if (!noRecDevice)
	{
		//length = length / sizeof(float);

		if (length > 8192)
		{
			WriteToLog("Rec buffer overflow, clamping the callback's length.");
			length = 8192;
		}

		int lenrest = length % 8;
		int len8 = length - lenrest;

		__m256 recVol_intr;
		recVol_intr.m256_f32[0] = recDeviceVolume;
		recVol_intr.m256_f32[1] = recDeviceVolume;
		recVol_intr.m256_f32[2] = recDeviceVolume;
		recVol_intr.m256_f32[3] = recDeviceVolume;
		recVol_intr.m256_f32[4] = recDeviceVolume;
		recVol_intr.m256_f32[5] = recDeviceVolume;
		recVol_intr.m256_f32[6] = recDeviceVolume;
		recVol_intr.m256_f32[7] = recDeviceVolume;


		for (int i = 0; i < len8; )
		{
			__m256 fvi_intr = _mm256_load_ps(fbuf + i);

			//float fvi = fbuf[i];

			//
			fvi_intr.m256_f32[0] = FixDeviceBit(fvi_intr.m256_f32[0]);
			fvi_intr.m256_f32[1] = FixDeviceBit(fvi_intr.m256_f32[1]);
			fvi_intr.m256_f32[2] = FixDeviceBit(fvi_intr.m256_f32[2]);
			fvi_intr.m256_f32[3] = FixDeviceBit(fvi_intr.m256_f32[3]);
			fvi_intr.m256_f32[4] = FixDeviceBit(fvi_intr.m256_f32[4]);
			fvi_intr.m256_f32[5] = FixDeviceBit(fvi_intr.m256_f32[5]);
			fvi_intr.m256_f32[6] = FixDeviceBit(fvi_intr.m256_f32[6]);
			fvi_intr.m256_f32[7] = FixDeviceBit(fvi_intr.m256_f32[7]);

			//if (recDeviceVolume < 1.0f)
			//{
			fvi_intr = _mm256_mul_ps(fvi_intr, recVol_intr);
				/*fvi_intr.m256_f32[0] *= recDeviceVolume;
				fvi_intr.m256_f32[1] *= recDeviceVolume;
				fvi_intr.m256_f32[2] *= recDeviceVolume;
				fvi_intr.m256_f32[3] *= recDeviceVolume;
				fvi_intr.m256_f32[4] *= recDeviceVolume;
				fvi_intr.m256_f32[5] *= recDeviceVolume;
				fvi_intr.m256_f32[6] *= recDeviceVolume;
				fvi_intr.m256_f32[7] *= recDeviceVolume;
				*/
			//}
			//fvi *= recDeviceVolume;

			//CheckRecorderPeak(fvi);

			CheckRecorderPeak(fvi_intr.m256_f32[0]);
			CheckRecorderPeak(fvi_intr.m256_f32[1]);
			CheckRecorderPeak(fvi_intr.m256_f32[2]);
			CheckRecorderPeak(fvi_intr.m256_f32[3]);
			CheckRecorderPeak(fvi_intr.m256_f32[4]);
			CheckRecorderPeak(fvi_intr.m256_f32[5]);
			CheckRecorderPeak(fvi_intr.m256_f32[6]);
			CheckRecorderPeak(fvi_intr.m256_f32[7]);

			//prec->recbuf[prec->bufpos++] = fvi;
			_mm256_store_ps(&prec->rectempbuf[prec->temppos], fvi_intr);

			//prec->bufpos += 8;
			prec->temppos += 8;
			i += 8;
			
			if (prec->temppos >= 256)
			{
				RecProcess256Samples();
			}
			
			if (prec->bufpos >= RECBUFSTANDARDSIZE - 1)
			{
				prec->breakrec = TRUE;
				return false;
			}
		}
		for (int i = len8; i < length; ++i)
		{
			float fvi = fbuf[i];
			fvi = FixDeviceBit(fvi);
			fvi *= recDeviceVolume;
			CheckRecorderPeak(fvi);

			prec->rectempbuf[prec->temppos++] = fvi;

			if (prec->temppos >= 256)
			{
				RecProcess256Samples();
			}

			if (prec->bufpos >= RECBUFSTANDARDSIZE - 1)
			{
				prec->breakrec = TRUE;
				return false;
			}
		}
	}

	return true;
}

float CalcSplineInterpolation(float v0, float v1, float v2, float pos)
{
	float cp = 2 * v1 - (v0 + v2) / 2.0f;
	float out = sqrtf(1.0f - pos) * v0 + 2 * (1 - pos) * pos * cp + sqrtf(pos) * v2;
	return cp;
}

static float proc256prevbuf = 0.0f;

void RecProcess256Samples()
{
	if (baserec.temppos < 256)
		return;

	_m_prefetchrs(baserec.rectempbuf);

	int recslowdownsamples = RECSLOWDOWNSAMPLES;
	if (stretchSamples > 0)
		recslowdownsamples = 256 + stretchSamples;

	//disable
	//recslowdownsamples = 256;

	for (int i = 0; i < recslowdownsamples; ++i)
	{
		float src_idx = ((float)i * 256.0f / (float)(recslowdownsamples));
		int idx = (int)roundf(src_idx);
		int idxprev = (int)roundf(src_idx - 1.0f);
		int idxnext = (int)roundf(src_idx + 1.0f);
		float spos = 0.5f + (float)(recslowdownsamples) / 256.0f - 1.0f;
		
		float vclean = baserec.rectempbuf[idx];

		float v1safe = 0;
		if (idxprev >= 0)
			v1safe = baserec.rectempbuf[idxprev];
		else
			v1safe = proc256prevbuf;

		float v2safe = 0;
		if (idxnext < 256)
			v2safe = baserec.rectempbuf[idxnext];
		else
			v2safe = vclean;

		float fintrp = CalcSplineInterpolation(v1safe, vclean, v2safe, spos);

		float fv = vclean +fintrp * 0.1f;

		//fv += 0.16f;
		
		baserec.recbuf[baserec.bufpos++] = fv;
	}

	//memmove(baserec.rectempbuf, baserec.rectempbuf + 256, (baserec.temppos - 256) * sizeof(float));
	
	proc256prevbuf = baserec.rectempbuf[baserec.temppos - 1];

	baserec.temppos -= 256;

	if (baserec.temppos > 0)
	{
		for (int i = 0; i < baserec.temppos; ++i)
		{
			baserec.rectempbuf[i] = baserec.rectempbuf[i + 256];
		}
		//baserec.rectempbuf[baserec.temppos] = 0.0;
	}
}

void RecorderSetCallbacks()
{
	gaudio->SetCallbacks(RecorderPushData, RecorderPullData);
}

void InitRec(Record& rec)
{
	//InitFolders();
	//_mkdir(getProgramFileName(""));

	rec.temppos = 0;
	rec.bufpos = 0;
	rec.unipos = 0;
	rec.playpos = 0;
	rec.writepos = 0;
	rec.breakrec = FALSE;
	rec.peak = 0.0f;
	rec.peakpos = 0;
	ZeroMemory(rec.safebuf_before, sizeof(rec.safebuf_before));
	ZeroMemory(rec.recbuf, sizeof(rec.recbuf));
	ZeroMemory(rec.safebuf_after, sizeof(rec.safebuf_after));

	RecorderSetCallbacks();
}

int recOrStop = 1;

RIAAFilter baseriaafilter;
RIAAFilter baseinvriaafilter;

void PrepareRIAAFilter(RIAAFilter& filter, double t1, double t2, double t3 )
{
	filter.z1_state = 0.0;
	filter.z2_state = 0.0;

	double p1 = t3 / 1000000.0f; // 50.05 Hz
	double p2 = t1 / 1000000.0f;   // 2212 Hz
	double z1 = t2 / 1000000.0f;  // 500.5 Hz

	double pole1 = std::exp(-1.0 / (RECSAMPLERATE * p1));
	double pole2 = std::exp(-1.0 / (RECSAMPLERATE * p2));
	double zero1 = std::exp(-1.0 / (RECSAMPLERATE * z1));

	// Simple 1st/2nd order combination mapping
	filter.b0 = 1.0;
	filter.b1 = -zero1;
	filter.b2 = 0.0;

	filter.a1 = -pole1 - pole2;
	filter.a2 = pole1 * pole2;
}


double ProcessRIAAFilter(RIAAFilter& filter, double in)
{
	// Direct Form II realization
	double w = in - filter.a1 * filter.z1_state - filter.a2 * filter.z2_state;
	double y = filter.b0 * w + filter.b1 * filter.z1_state + filter.b2 * filter.z2_state;

	filter.z2_state = filter.z1_state;
	filter.z1_state = w;

	return y;
}

void PrepareInverseRIAAFilter(RIAAFilter& filter, double t1, double t2, double t3)
{
	double p1 = t3 / 1000000.0f; // 50.05 Hz
	double p2 = t1 / 1000000.0f;   // 2212 Hz
	double z1 = t2 / 1000000.0f;  // 500.5 Hz

	double pole1 = std::exp(-1.0 / (RECSAMPLERATE * p1));
	double pole2 = std::exp(-1.0 / (RECSAMPLERATE * p2));
	double zero1 = std::exp(-1.0 / (RECSAMPLERATE * z1));

	// Simple 1st/2nd order combination mapping
	filter.b0 = 1.0;
	filter.b1 = -zero1;
	filter.b2 = 0.0;

	filter.a1 = -pole1 - pole2;
	filter.a2 = pole1 * pole2;

	filter.x2 = 0.0;
	filter.x1 = 0.0;
	filter.y2 = 0.0;
	filter.y1 = 0.0;
}

double ProcessInverseRIAAFilter(RIAAFilter& filter, double in)
{
	// Standard Direct Form I calculation: 
	// y[n] = (b0*x[n] + b1*x[n-1] + b2*x[n-2] - a1*y[n-1] - a2*y[n-2]) / a0
	double out = (filter.b0 * in + filter.b1 * filter.x1 + filter.b2 * filter.x2
		- filter.a1 * filter.y1 - filter.a2 * filter.y2);

	// Update delay line history
	filter.x2 = filter.x1;
	filter.x1 = in;
	filter.y2 = filter.y1;
	filter.y1 = out;

	return out;
}

void HelperProcessRIAAFilter(Record& rec, RIAAFilter& filter)
{
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float in = rec.recbuf[i];
		float out;
		out = ProcessRIAAFilter(filter, in);
		out *= 0.166f;
		float mix = in * (1.0f - recQFX) + out * recQFX;
		rec.recbuf[i] = mix;
	}
}

void HelperProcessInverseRIAAFilter(Record& rec, RIAAFilter& filter)
{
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float in = rec.recbuf[i];
		float out;
		out = ProcessInverseRIAAFilter(filter, in);
		out *= 0.0166f;
		float mix = in * (1.0f - recQFX) + out * recQFX;
		rec.recbuf[i] = mix;
	}
}



void PostProcessRec(Record& rec) {
	PrepareRIAAFilter(baseriaafilter, 30, 150, 2000);
	rec.peak = 0;
	rec.peakpos = 0;

	float remasterq = 1.0f;
//	if (IsRecorderRemastering())
//		remasterq = 0.01f;

	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fsample = rec.recbuf[i];
		float ffilter = 0;
		ffilter = ProcessRIAAFilter(baseriaafilter, fsample);
		ffilter *= 0.166f * (recQFX + 0.5f);
		ffilter *= remasterq;
		fsample = (fsample + ffilter) / 2.0f;
		// fadeout
		if (i > (rec.bufpos - 24000))
			fsample *= (rec.bufpos - i) / 24000.0;

		CheckRecorderPeak(fsample, i);

		rec.recbuf[i] = fsample;
	}
}

void StartRec(Record& rec) {
	if (recOrPlay < 0)
		return;

	undolevel = 0;

	WriteToLog("Record started... ticktime: %i", timeGetTime());

	ZeroMemory(rec.recbuf, RECBUFSTANDARDSIZE * sizeof(float));
	rec.temppos = 0;
	rec.bufpos = 0;
	rec.writepos = 0;
	rec.breakrec = FALSE;
	rec.peak = 0.0f;
	rec.peakpos = 0;
	rec.lastreccallback_time = timeGetTime();

	PrepareInverseRIAAFilter(baseinvriaafilter, 68, 250, 2000);

	rec.tape = fopen(getProgramFileName("tape.bin"), "wb");
	rec.tapepolarized = fopen(getProgramFileName("tape_polarized.bin"), "wb");

	noRecDevice = false;

	gaudio->Rec();

	recOrPlay = -1;
}


Record prevrec[(RECMAXUNDOLEVEL + 4)];

int undolevel = 0;

void UndoFXFromRec()
{
	if (undolevel <= 0)
		return;

	baserec = prevrec[--undolevel];
}

void AddRIAAToRec(Record& rec)
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	PrepareRIAAFilter(baseriaafilter, 68, 200, 2000);
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fsample = rec.recbuf[i];
		float friaa = 0;
		friaa = ProcessRIAAFilter(baseriaafilter, fsample);
		friaa *= 0.166f * recQFX;
		fsample = (fsample + friaa) / 1.66f;
		CheckRecorderPeak(fsample, i);
		rec.recbuf[i] = fsample;
	}
}

float RecCalculatePeak(Record& rec, int begin, int end)
{
	if (begin < 0)
		return 0.0f;
	if (begin >= rec.bufpos)
		return 0.0f;
	if (end < 0)
		end = rec.bufpos;
	if (end > rec.bufpos)
		end = rec.bufpos;
	float fmaxv = 0;
	int maxpos = 0;
	for (int i = begin; i < end; ++i)
	{
		float fsample = abs(rec.recbuf[i]);
		if (fsample > fmaxv)
		{
			fmaxv = fsample;
			maxpos = i;
		}
	}
	rec.peak = fmaxv;
	rec.peakpos = maxpos;
	return fmaxv;
}

void NormalizeRec(Record& rec)
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	float fmaxv = 0;
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fsample = abs(rec.recbuf[i]);
		if (fsample > fmaxv)
			fmaxv = fsample;
	}
	float fmult = 0;
	fmult = 1.0f / fmaxv;
	fmult *= 0.999f;
	fmult *= ((float)adcIntMaxNatural / 32767.0f);
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fv = rec.recbuf[i];		
		fv *= fmult;
		rec.recbuf[i] = fv;
	}
	rec.peak = 0.999f * (float)adcIntMaxNatural / 32767.0f;
}

void LimitRec(Record& rec)
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fv = rec.recbuf[i];

		float fmult2 = 1.0f;
		const float limiter_peak = 0.7f;
		if (fabs(fv) > limiter_peak)
		{
			fmult2 = (fabs(fv) - limiter_peak) * 0.66f * (1.0f - recQFX) / 2.0f;
			if (fv > 0)
				fv = limiter_peak + fmult2;
			else
				fv = -limiter_peak - fmult2;
		}

		rec.recbuf[i] = fv;
	}
	float fmaxv = 0;
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fsample = abs(rec.recbuf[i]);
		if (fsample > fmaxv)
			fmaxv = fsample;
	}
	rec.peak = fmaxv;
}


void ReadRec()
{
	if (remastermode == 0)
	{
		baserec.tape = fopen(getProgramFileName("tape_remaster.bin"), "rb");
		baserec.tapepolarized = 0;
	}
	else
	{
		baserec.tape = fopen(getProgramFileName("tape.bin"), "rb");
		baserec.tapepolarized = fopen(getProgramFileName("tape_polarized.bin"), "rb");
	}

	int readpos = 0;
	//baserec.peak = 0;
	//baserec.peakpos = 0;
	while (readpos < baserec.bufpos)
	{
		int rlen = 1024;
		int rdif = baserec.bufpos - readpos;
		if (rdif < rlen)
			rlen = rdif;

		short sv[1024];
		short svp[1024];
		fread(sv, 2, rlen, baserec.tape);
		if(baserec.tapepolarized != 0)
			fread(svp, 2, rlen, baserec.tapepolarized);

		for (int i = 0; i < rlen; ++i)
		{
			float fv = 0, fvp = 0;
			fv = -PolarizeFloat(Int16ToFloat(UnpackInt16ForReading(sv[i])), false, false);
			if (baserec.tapepolarized != 0)
			{
				fvp = PolarizeFloat(Int16ToFloat(UnpackInt16ForReading(svp[i])), true, false);
			}
			else
			{
				fvp = 0.0f;
			}
			float fvv = baserec.recbuf[readpos];
			baserec.recbuf[readpos] = (fv * 0.7f + fvp * 0.3f) - fvv * 0.1f;
			readpos += 1;
		}
	}
	fclose(baserec.tape);
	if(baserec.tapepolarized != 0)
		fclose(baserec.tapepolarized);


	RecorderReRender();
}

void StopRec(Record& rec)
{
	if (recOrPlay >= 0)
		return;

	//WriteRec(true);

	WriteToLog("Record stopped... ticktime: %i", timeGetTime());

	rec.breakrec = FALSE;

	remastermode = 1;

	recOrPlay = 0;

	recOrStop = 1;

	fclose(rec.tape);
	fclose(rec.tapepolarized);

	gaudio->Stop();

	//BASS_StreamFree(rec.record);

	ReadRec();

	//if (! noRecDevice)
	//{
	//}
	PostProcessRec(rec);
}

int playOrStop = 1;

void StopPlayingRec()
{
	if (recOrPlay == 0)
		return;

	WriteToLog("Record stopped playing..., timticks: %i", timeGetTime());

	recOrPlay = 0;
	playOrStop = 1;

	streamrec.playpos = 0;
	baserec.unipos = 0;
	streamrec.breakplay = TRUE;

	gaudio->Stop();

	streamrec.stream = 0;
}

bool RecorderPullData(float* buffer, int length)
{
	if(remastermode != 0)
		recOrPlay = 1;

	if (streamrec.breakplay == TRUE)
	{
		return false;
	}

	streamrec.playcallback_dt = timeGetTime() - streamrec.lastplaycallback_time;
	streamrec.lastplaycallback_time = timeGetTime();

	int recspeedupsamples = 256;
	float fmult = 1.0f;
	
	if (IsRecorderRemastering())
	{
		//fmult = 0.66f;
		recspeedupsamples = RECSLOWDOWNSAMPLES;
		if (stretchSamples > 0)
			recspeedupsamples = 256 + stretchSamples;
	}

	float* fbuf = (float*)buffer;

	for (int i = 0; i < length; ++i)
	{
		float fi = (float)i * (float)(recspeedupsamples) / 256.0f;
		int src_idx = (int)roundf(fi);
		src_idx += streamrec.playpos;

		int idxprev = (int)roundf(fi - 1.0);
		if (idxprev < 0)
			idxprev = 0;

		idxprev += streamrec.playpos;

		float fv = streamrec.recbuf[(int)src_idx];

		float fvprev = streamrec.recbuf[idxprev];

		float midpos = 256.0f / (float)recspeedupsamples;

		float finterp = fv * midpos + fvprev * (1.0f - midpos);
			
		fbuf[i] = ( fv + finterp ) / 2.0f;

		//streamrec.playpos = (int)src_idx;
		
		//streamrec.playpos += 1;

		//if (baserec.playpos >= RECBUFSTANDARDSIZE)
		//	return BASS_STREAMPROC_END;
		if (src_idx >= streamrec.bufpos + 4800)
		{
			if(playloop && remastermode != 0)
			{
				streamrec.playpos = 0;
			}
			else
			{
				streamrec.breakplay = TRUE;
				return false;
			}
		}
	}
	streamrec.playpos += (int)roundf((float)length * (float)recspeedupsamples / 256.0f);
	return true;
}

void PlayRec()
{
	if (recOrPlay > 0)
		return;

	WriteToLog("Record started playing... timeticks: %i", timeGetTime());

	streamrec = baserec;

	recOrPlay = 1;
	streamrec.breakplay = FALSE;
	streamrec.playpos = 0;
	baserec.unipos = 0;
	streamrec.lastplaycallback_time = timeGetTime();
	streamrec.playcallback_dt = 4800;

	gaudio->Play();
}


float ComputeAverage(Record& rec, int from, int to, int step)
{
	double sum = 0;
	for (unsigned i = from; i < to; i += step)
	{
		float v = rec.recbuf[i];
		sum += v;
	}
	sum /= (double)((to - from) / step);
	return sum;
}

void ComputeWaveform(Record& rec, int from, int to, int step, float& outplus, float& outminus)
{
	double maxplus = 0;
	double maxminus = 0;
	for (int i = from; i < to; i += step)
	{
		float v = rec.recbuf[i];
		if (v > 0)
		{
			if (v > maxplus)
				maxplus = v;
		}
		else
		{
			if (v < maxminus)
				maxminus = v;
		}
	}
	outplus = maxplus;
	outminus = maxminus;
}

float ComputeRMS(Record& rec, int from, int to, int step)
{
	if (from < 0)
		from = 0;
	if (to > RECBUFSTANDARDSIZE - 1)
		to = RECBUFSTANDARDSIZE - 1;
	if (from == to)
	{
		return 0.0;
	}
	double squaresum = 0;
	for (unsigned i = from; i < to; i += step)
	{
		float v = abs(rec.recbuf[i]);
		squaresum += v * v;
	}
	double meanofsquares = squaresum / (double)((to - from) / step);
	return sqrtf(meanofsquares);
}

void SaveRec(bool wavormp3)
{
	//_mkdir(getAppFileName("output/"));

	FILE* counterfile = fopen(getMusicFileName("output/recoutcounter.bin"), "a + b");
	unsigned reccount = 0;

	bool dontseek = true;

	if (fseek(counterfile, -4, SEEK_END) == 0)
	{
		fread(&reccount, 4, 1, counterfile);
		dontseek = false;
	}

	if(wavormp3)
		reccount += 1;

	/*HSTREAM writestream = BASS_StreamCreate(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, STREAMPROC_DUMMY, 0);
		HENCODE encoder = BASS_Encode_StartPCMFile(writestream, BASS_ENCODE_AUTOFREE, "output.wav");
		BASS_Encode_Write(encoder, baserec.recbuf, baserec.bufpos * sizeof(float));
		BASS_Encode_Stop(encoder);
		BASS_StreamFree(writestream);
		*/

	if (wavormp3)
	{
#ifdef RECUSELIBAUDIOFILE
		AudioFile<float> afile;
		afile.setNumChannels(1);
		afile.setSampleRate(RECSAMPLERATE);
		afile.setNumSamplesPerChannel(baserec.bufpos);
		for (int i = 0; i < baserec.bufpos; ++i)
			afile.samples[0][i] = baserec.recbuf[i];

		static char recoutfilename[1024];
		sprintf(recoutfilename, getMusicFileName("output/RECOUT%04i.wav"), reccount);

		afile.save(recoutfilename);

		static char savemessage[1024];
		sprintf(savemessage, "Recording saved to %s !", recoutfilename);

#else
		static char savemessage[1024];
		sprintf(savemessage, "WAV export was switched off in this version of the app");
#endif
		MessageBox(0, savemessage, "Saved!", MB_ICONINFORMATION | MB_OK);
	}
	else
	{
#ifdef RECUSELIBLAME
		static char recoutfilename[1024];
		sprintf(recoutfilename, getMusicFileName("output/RECOUT%04i.mp3"), reccount);

		std::ofstream outfile;
		outfile.open(recoutfilename, std::ios_base::binary);

		lame_global_flags* gf = lame_init();

		if (gf == NULL) {
			outfile.close();
			WriteToLog("Failed to init LAME encoder.");
			return;
		}

		lame_set_mode(gf, MPEG_mode::MONO);
		lame_set_quality(gf, 0);
		lame_set_num_channels(gf, 1);
		lame_set_in_samplerate(gf, 48000);
		lame_set_brate(gf, 256);

		static char recordtitle[256];
		sprintf(recordtitle, "Record %04i", reccount);

		id3tag_init(gf);
		id3tag_add_v2(gf);
		id3tag_set_title(gf, recordtitle);
		id3tag_set_artist(gf, exportartist.c_str());

		bool retVal = lame_init_params(gf);

		if (retVal < 0) {
			WriteToLog("Failed to init LAME encoder parameters.");
			return;
		}

		unsigned char mp3Buffer[96000];
		float inbuf[1024];
		int out_write;
		for(int i=0; i < baserec.bufpos; )
		{
			// pack for encoder
			for (int j = 0; j < 1024; ++j)
			{
				float fv = baserec.recbuf[i + j];
				if (fv > 0)
					fv *= 32767.0f;
				if (fv < 0)
					fv *= 32768.0f;
				//fv *= 32768.0f / 32767.0f;
				inbuf[j] = fv;
			}
			out_write = lame_encode_buffer_float(gf, inbuf, NULL, 1024, mp3Buffer, sizeof(mp3Buffer));
			if (out_write > 0) {
				outfile.write((char*)(mp3Buffer), out_write);
			}
			i += 1024;
		}

		outfile.close();
		lame_close(gf);

		static char savemessage[1024];
		sprintf(savemessage, "Recording saved to %s !", recoutfilename);

#else 
		static char savemessage[1024];
		sprintf(savemessage, "MP3 export was switched off in this version of the app.");
#endif

		MessageBox(0, savemessage, "Saved!", MB_ICONINFORMATION | MB_OK);
	}

	if (!dontseek)
		fseek(counterfile, -4, SEEK_END);

	fwrite(&reccount, 4, 1, counterfile);

	fflush(counterfile);

	fclose(counterfile);

}

static float previn = 0, prevout = 0;

void PreprocessRec(Record& rec, int preprocesslen)
{
	if (rec.writepos == 0)
	{
		previn = 0;
		prevout = 0;
	}
	for (int i = rec.writepos; i < rec.writepos + preprocesslen; ++i)
	{
		float fv = rec.recbuf[i];
		fv = ExorciseFloatNoise(fv);
		rec.recbuf[i] = fv;
	}
	float coeff = 0.98f;
	float gain = (1.0f + coeff) / 2.0f;
	for (int i = rec.writepos; i < rec.writepos + preprocesslen; ++i)
	{
		float fv = rec.recbuf[i];
		float out = (fv - previn) * gain + coeff * prevout;
		rec.recbuf[i] = out;
		previn = fv;
		prevout = out;
	}
}


#define RECWRITECHUNK 128
#define RECWRITECHUNKLONG 4096
#define RECREWRITECOUNT 3
//#define RECWRITENEEDLECHUNK 

//int recTapeRewrites = RECREWRITECOUNT;

void WriteRec(bool forcerec)
{
	if (!bDoRewrites && !forcerec)
	{
		// include write delay
		if (baserec.bufpos - baserec.writepos < 48000)
			return;
	}

	if (IsRecorderWriting())
	{
		//float fv[RECWRITECHUNKLONG];

		short sv[RECREWRITECOUNT][RECWRITECHUNKLONG];
		short svp[RECREWRITECOUNT][RECWRITECHUNKLONG];

		while(baserec.writepos < baserec.bufpos)
		{
			int wlen = bDoRewrites ? RECWRITECHUNK : RECWRITECHUNKLONG;
			int wdif = baserec.bufpos - baserec.writepos;
			if (wdif < wlen)
				wlen = wdif;
			if (wlen > RECWRITECHUNK)
				wlen = RECWRITECHUNK;

			PreprocessRec(baserec, wlen);
			
			float remasterq = 1.0f;
//			if (IsRecorderRemastering())
//				remasterq = 0.01f;

			for (int i = 0; i < wlen; ++i)
			{
				float fvi = baserec.recbuf[baserec.writepos + i];
				//fvi = fvi;

				float fveff = ProcessInverseRIAAFilter(baseinvriaafilter, fvi);
				fveff *= 0.0166f * (recQFX + 0.5f);

				if (!bDoRewrites)
					fveff *= 0.1f;
				
				fveff *= remasterq;

				fvi = (fvi + fveff) / 2.0f;

				fvi += 0.000001f;

				//fv[i] = fvi;

				for (int j = 0; j < RECREWRITECOUNT; ++j)
				{
					sv[j][i] = PackInt16ForWriting(FloatToInt16(PolarizeFloat(-fvi, false, true)));
					svp[j][i] = PackInt16ForWriting(FloatToInt16(PolarizeFloat(fvi, true, true)));
					fvi -= 0.000000166f;
				}
			}

			// standard 

			if (bDoRewrites)
			{
				fwrite(sv[0], 2, wlen, baserec.tape);
			}
			else
			{
				fwrite(sv[0], 2 * wlen, 1, baserec.tape);
			}

			if (bDoRewrites)
			{
				for (int i = 1; i < RECREWRITECOUNT; ++i)
				{
					fseek(baserec.tape, -wlen * 2, SEEK_CUR);
					fwrite(sv[i], 2, wlen, baserec.tape);
				}
			}
			if(bDoRewrites)
				fflush(baserec.tape);

			// polarized 

			if (baserec.tapepolarized != 0)
			{

				if (bDoRewrites)
				{
					fwrite(svp[0], 2, wlen, baserec.tapepolarized);
				}
				else
				{
					fwrite(svp[0], 2 * wlen, 1, baserec.tapepolarized);
				}
				if (bDoRewrites)
				{
					for (int i = 1; i < RECREWRITECOUNT; ++i)
					{
						fseek(baserec.tapepolarized, -wlen * 2, SEEK_CUR);
						fwrite(svp[i], 2, wlen, baserec.tapepolarized);
					}
				}
				if (bDoRewrites)
					fflush(baserec.tapepolarized);
			}

			baserec.writepos += wlen;
		}
	}
}

void RecorderDebugCheck()
{
	if (baserec.bufpos < 0)
		baserec.bufpos = 0;

	if (baserec.bufpos > RECBUFSTANDARDSIZE)
		baserec.bufpos = RECBUFSTANDARDSIZE;

	if (streamrec.playpos < 0)
		streamrec.playpos = 0;

	if (streamrec.playpos > RECBUFSTANDARDSIZE)
		streamrec.playpos = RECBUFSTANDARDSIZE;

	if (baserec.unipos < 0)
		baserec.unipos = 0;

	if (baserec.unipos > RECBUFSTANDARDSIZE)
		baserec.unipos = RECBUFSTANDARDSIZE;

	// clamp to 2.0

	for (int i = 0; i < baserec.bufpos; ++i)
	{
		float fv = baserec.recbuf[i];
		if (fv > 2.0f)
		{
			fv = 2.0f;
		}
		if (fv < -2.0f)
		{
			fv = -2.0f;
		}
		baserec.recbuf[i] = fv;
	}

	// debug check qfx
	if (recQFX < 0.1f)
		recQFX = 0.1f;
	if (recQFX > 1.0f)
		recQFX = 1.0f;

}

float dummysinus = 0;

void ResetDummySinus()
{
	dummysinus = 0;
}

float GetDummySinus() {
	float out = sinf(dummysinus);
	dummysinus += 1.0f / 40.0f;
	return out;
}

float checkzeroinput = 0.0f;

extern int curDevNum, curRecDevNum;

void RecorderUpdate(float dt)
{
	//BASS_Update(10);
	//BASS_ChannelUpdate(baserec.record, 10);

	if (IsRecorderWriting())
	{
		if (baserec.lastreccallback_time != 0)
		{
			/*
			if (timeGetTime() - baserec.lastreccallback_time > 6000)
			{
				MessageBox(mainWnd, "If your recording device breaks the stream, we can't fix it because we're using a little simpler libraries.", "Notice", MB_OK);
				if (remastermode == 0)
				{
					StopRemaster();
				}
				else
				{
					StopRec(baserec);
				}
			}*/
		}
	}

	if (IsRecorderRemastering())
	{
		//remastering fade in and fade out
		if(baserec.bufpos < 48000)
			recDeviceVolume = baserec.bufpos / 48000.0;
		else
		if(baserec.bufpos > streamrec.bufpos - 48000)
		{
			recDeviceVolume = (streamrec.bufpos - baserec.bufpos) / 48000.0;
			if (recDeviceVolume > 1.0)
				recDeviceVolume = 1.0;
		}
		else {
			recDeviceVolume = 1.0f;
		}

		recDeviceVolume *= 2.66f;
		baserec.unipos = baserec.bufpos;
		if (streamrec.breakplay == TRUE)
		{
			StopRemaster();
		}
		masterDevicePlayTime += dt;
	}
	else
	if (recOrPlay < 0)
	{
		int checksamples = 4096;
		if (baserec.bufpos > checksamples)
		{
			for (int i = 0; i < checksamples; ++i)
			{
				float fv = baserec.recbuf[i];
				if (fv > checkzeroinput)
				{
					checkzeroinput = fv;
				}
			}

			if (checkzeroinput == 0.0f)
				;//noRecDevice = true;
		}

		if (noRecDevice)
		{
			int dummysamples = dt * 48000.0f;

			if (recOrPlay < 0)
			{
				for (int i = 0; i < dummysamples; ++i)
				{
					float fv = GetDummySinus() * recDeviceVolume * 0.33f;
					CheckRecorderPeak(fv);
					//baserec.recbuf[baserec.bufpos++] = fv;
				}
			}
			/*if (remastermode == 0)
			{
				for (int i = 0; i < dummysamples; ++i)
				{
					float fv = streamrec.recbuf[streamrec.playpos + i];
					baserec.recbuf[baserec.bufpos++] = fv;
				}
			}*/
		}

		recOrStop = 0;

		float rvolume = recDeviceRecTime / recFadeInTime;
		if (rvolume > 1.0f)
			rvolume = 1.0f;

		baserec.unipos = baserec.bufpos;

		recDeviceVolume = rvolume;
		recDeviceRecTime += dt;

		if (baserec.breakrec == TRUE)
		{
			StopRec(baserec);
		}
	}
	else if (recOrPlay > 0)
	{
		playOrStop = 0;

		baserec.unipos = streamrec.playpos;
		//BASS_ChannelUpdate(baserec.stream, 10);

		if (streamrec.breakplay == TRUE)
			StopPlayingRec();
	}
	else
	{
		recDeviceVolume = 0;
		recDeviceRecTime = 0;
		masterDevicePlayTime = 0;
		baserec.unipos = 0;
		ResetDummySinus();
		checkzeroinput = 0.0f;
	}
	//BASS_Update(100);
}

bool CheckRecorderPeak(float in, int bufpos)
{
	float absv = fabs(in);
	if (absv > baserec.peak)
	{
		baserec.peak = absv;
		if (bufpos < 0)
			bufpos = baserec.bufpos;
		baserec.peakpos = bufpos;
		return true;
	}
	return false;
}

//DWORD error;

int remastermode = 1;

void StartRemaster()
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	WriteToLog("Remaster started..., timeticks: %i", timeGetTime());


	remastermode = 0;

	// play streamrec
	streamrec = baserec;

	//CopyMemory(streamrec.recbuf, baserec.recbuf, sizeof(baserec.recbuf));

	ZeroMemory(streamrec.recbuf, 48000 * sizeof(float));

	memcpy(streamrec.recbuf + 48000, recbeep, RECBEEPLENMAX * sizeof(float));

	memcpy(streamrec.recbuf + 48000 + RECBEEPLENMAX, baserec.recbuf, baserec.bufpos * sizeof(float));

	ZeroMemory(streamrec.recbuf + baserec.bufpos + 48000 + RECBEEPLENMAX, 48000 * sizeof(float));
	streamrec.bufpos += 96000 + RECBEEPLENMAX;

	// 
	//streamrec.bufpos += 48000 * 32;

	ZeroMemory(baserec.recbuf, RECBUFSTANDARDSIZE * sizeof(float));
	
	streamrec.breakplay = FALSE;
	streamrec.playpos = 0;
	baserec.unipos = 0;

	streamrec.lastplaycallback_time = timeGetTime();
	streamrec.playcallback_dt = 4800;
	
	//////////////////////////
	// rec baserec
	
	baserec.bufpos = 0;
	baserec.unipos = 0;
	baserec.playpos = 0;
	baserec.writepos = 0;
	baserec.breakrec = FALSE;
	baserec.peak = 0.0f;
	baserec.peakpos = 0;
	baserec.lastreccallback_time = timeGetTime();

	PrepareInverseRIAAFilter(baseinvriaafilter, 68, 250, 2000);

	baserec.tape = fopen(getProgramFileName("tape_remaster.bin"), "wb");

	noRecDevice = false;

	//BASS_RecordFree();

	//BASS_RecordInit(-1);


	gaudio->Remaster();

	////////////////////

	//////////////
	recOrStop = 0;
	playOrStop = 0;
}

void StopRemaster()
{
	WriteToLog("Remaster stopped..., timeticks: %i", timeGetTime());

	// stop record

	baserec.breakrec = TRUE;
	streamrec.breakplay = TRUE;

	fclose(baserec.tape);

	gaudio->Stop();

	baserec.record = 0;

	ReadRec();

	PostProcessRec(baserec);

	memcpy(recbeepregion, baserec.recbuf, 96000 * sizeof(float));

	RecorderFixRemasterOffset();

	//gaudio->Stop();

	recOrPlay = 0;
	recOrStop = 1;
	playOrStop = 1;

	remastermode = 1;
}

bool IsRecorderPlaying()
{
	if (remastermode == 0)
	{
		return true;
	}
	else
	{
		return (recOrPlay > 0);
	}
}

bool IsRecorderWriting()
{
	if (remastermode == 0)
	{
		return true;
	}else{
		return (recOrPlay <0);
	}
}

bool IsRecorderIdling()
{
	if (remastermode == 0)
	{
		return false;
	}

	return recOrPlay == 0;
}

bool IsRecorderRemastering() {
	return remastermode == 0;
}

bool RecorderDeviceRetrieve(int devNum, int recDevNum)
{
//	return false;
//	if (devNum == 0)
//		devNum = -1;
	WriteToLog("Retrieving audio device: devnum %i, recdevnum %i", devNum, recDevNum);
	if( IsRecorderWriting())
		StopRec(baserec);
	if( IsRecorderPlaying())
		StopPlayingRec();
	if( IsRecorderRemastering())
		StopRemaster();

	remastermode = 1;
	recOrPlay = 0;
	recOrStop = 1;
	playOrStop = 1;
	baserec.breakrec = FALSE;
	baserec.breakplay = FALSE;
	streamrec.breakrec = FALSE;
	streamrec.breakplay = FALSE;
	baserec.playpos = 0;
	baserec.writepos = 0;
	streamrec.playpos = 0;
	streamrec.writepos = 0;
	baserec.unipos = 0;
	streamrec.unipos = 0;
	recDeviceVolume = 0;
	ZeroMemory(baserec.safebuf_before, sizeof(baserec.safebuf_before));
	ZeroMemory(streamrec.safebuf_before, sizeof(streamrec.safebuf_before));
	ZeroMemory(baserec.safebuf_after, sizeof(baserec.safebuf_after));
	ZeroMemory(streamrec.safebuf_after, sizeof(streamrec.safebuf_after));

	return gaudio->Retrieve(devNum, recDevNum);
}

bool RecorderCanUndo()
{
	return undolevel > 0;
}

bool RecorderCanDoFX()
{
	return undolevel < RECMAXUNDOLEVEL;
}

void RecorderSetLoop(bool loop)
{
	playloop = loop;
}

void RecorderSetRewriteMode(bool hddrewrite)
{
	//if(!IsRecorderWriting())
	bDoRewrites = hddrewrite;
	/*if (BASS_SetConfig(BASS_CONFIG_REC_BUFFER, bDoRewrites ? RECBUFFERDELAYMS : RECBUFFERLONGDELAYMS) != TRUE)
	{
		WriteToLog("BASS SetConfig failed at switching rewrite mode");
	}*/
}

void RecorderSetQFX(float qfxv)
{
	recQFX = qfxv;
	if (recQFX < 0.1f)
		recQFX = 0.1f;
	if (recQFX > 1.0f)
		recQFX = 1.0f;
}

float RecorderGetQFX()
{
	return recQFX;
}

long CalcMiddlePeakOffset(const float* buf, int len, float mult = 1.0f)
{
	float fvprev = 0.0f;
	bool gmove = false;
	int peaks[8192];
	int peaks_n = 0;
	for (int i = 0; i < len; ++i)
	{
		float fv = buf[i] * mult;
		bool newmove = false;
		if (fabs(fv) > 0.3f)
		{
			if (fv > fvprev)
			{
				newmove = true;
			}
			else
			if (fv < fvprev)
			{
				newmove = false;
			}
			if (newmove != gmove)
			{
				peaks[peaks_n++] = i;
				gmove = newmove;
				if (peaks_n >= 8192)
				{
					WriteToLog("Beep check peak overflow.");
					break;
				}
			}
		}
	}
	long goff = 0;
	for (int i = 0; i < peaks_n; ++i)
	{
		goff += peaks[i];
	}
	if (peaks_n > 0)
		goff /= peaks_n;

	return goff;
}

void RecorderLoadBeepSound(const float* buf, int len /* len to 8192 max */)
{
	if (len < 0)
		return;
	if (len > RECBEEPLENMAX)
	{
		WriteToLog("BEEP sound length is more than 8192 samples, clamping.");
		len = RECBEEPLENMAX;
	}


	ZeroMemory(recbeep, RECBEEPLENMAX * sizeof(float));

	for (int i = 0; i < len; ++i)
	{
		float fv = buf[i];
		if (fv < -1.0f)
			fv = -1.0f;
		if (fv > 1.0f)
			fv = 1.0f;
		recbeep[i] = fv;
	}
	recbeep_initialoffset = CalcMiddlePeakOffset(recbeep, 8192, 1.0f);
}

Record temprec;

void RecorderFixRemasterOffset()
{
	float regionmax = 0;
	for (int i = 0; i < 96000; ++i)
	{
		float fv = fabs(recbeepregion[i]);
		if (fv > regionmax)
			regionmax = fv;
	}
	float regionmult = 1.0f / regionmax;
	
	int remoffset = CalcMiddlePeakOffset(recbeepregion, 96000, regionmult);

	WriteToLog("Beep fix offset, standard: %i, current: %i", recbeep_initialoffset, remoffset);

	float fvmult = 0.0f;
	fvmult = RecCalculatePeak(streamrec, 48000 + RECBEEPLENMAX, -1);
	fvmult /= RecCalculatePeak(baserec, 96000 + RECBEEPLENMAX, -1);
	fvmult *= 0.7f;

	// mix with clean with QFX and offset
	for (int i = 0; i < baserec.bufpos; ++i)
	{
		int ioff_ideal = 48000 + recbeep_initialoffset;

		int ioff = remoffset;

		float sv = 0, fv = 0;

		int istream = ioff - ioff_ideal;

		if (istream >= 0)
		{
			sv = streamrec.recbuf[istream];
		}
		
		// invert or not??? must be a setting;
		fv = baserec.recbuf[i];

		//float fvmult = streamrec.peak / baserec.peak;

		baserec.recbuf[i] = fv * fvmult * recQFX + (1.0f - recQFX) * sv;
	}

	int cutout = 48000 + RECBEEPLENMAX;
	if (baserec.bufpos > cutout)
	{
		temprec = baserec;
		memcpy(baserec.recbuf, temprec.recbuf + cutout, (temprec.bufpos - cutout) * sizeof(float));
		ZeroMemory(baserec.recbuf + baserec.bufpos - cutout, cutout * sizeof(float));
		baserec.bufpos -= cutout;
	}
	for (int i = 0; i < 48000; ++i)
	{
		baserec.recbuf[i] *= ((float)i) / 48000.0f;
	}
	for (int i = 0; i < 8192; ++i)
	{
		baserec.recbuf[i] = 0;
	}
}

void RecorderSetColoration(int clr)
{
	if (clr < 0)
		clr = 0;
	if (clr > 2)
		clr = 2;

	int prevclr = recCLRFX;
	recCLRFX = clr;

	if (prevclr == clr)
		return;

	baserec.peak = 0;

	//rebuild coloration;
	BEGINHIDDENCODE(GetColorMultiplicator)
	for (int i = 0; i < baserec.bufpos; ++i)
	{
		float  fv = baserec.recbuf[i];
		fv /= GetColorMultiplicator(prevclr);
		fv *= GetColorMultiplicator(clr);
		CheckRecorderPeak(fv, i);
		baserec.recbuf[i] = fv;
	}
	ENDHIDDENCODE
}

void RecorderSetStretchSamples(int sn)
{
	if (sn >=0 && sn <= 10)
		stretchSamples = sn;
}

void RecRerenderTape(const char* tapemapname)
{
	int mapsize = RECBUFSTANDARDSIZE * sizeof(short);

	HANDLE hfile = CreateFileA(getProgramFileName(tapemapname), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_NO_BUFFERING | FILE_FLAG_WRITE_THROUGH, 0);

	if (hfile != INVALID_HANDLE_VALUE)
	{
		HANDLE hmap =CreateFileMappingA(hfile, 0, PAGE_READWRITE, 0, mapsize, 0);
		if (hmap != 0)
		{
			short* sbuf = (short*)MapViewOfFile(hmap, FILE_MAP_READ | FILE_MAP_WRITE, 0, 0, mapsize);
			
			if (sbuf != 0)
			{
				for (int i = 0; i < baserec.bufpos; ++i)
				{
					sbuf[i] = PackInt16ForWriting(FloatToInt16(baserec.recbuf[i]) - 1);
				}

				FlushViewOfFile(sbuf, mapsize);

				for (int i = 0; i < baserec.bufpos; ++i)
				{
					baserec.recbuf[i] = Int16ToFloat(UnpackInt16ForReading(sbuf[i])+1);
				}

				FlushViewOfFile(sbuf, mapsize);
			}
			else
			{
				WriteToLog("Failed to map view of file %i", GetLastError());
			}
			//sbuf[i] = UnpackInt16ForReading(sbuf[i]);
			UnmapViewOfFile(sbuf);
		}
		else
		{
			WriteToLog("Failed to map file");
		}
		CloseHandle(hmap);
	}
	else
	{
		WriteToLog("Failed to create tape file");
	}
	CloseHandle(hfile);
}

void RecorderReRender()
{
	RecRerenderTape("tape_render.bin");
}

void RecorderAddReverb()
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	ProcessReverb48khz(baserec.recbuf, baserec.bufpos, recQFX * 0.22f);
}

void RecorderHighCut(Record& rec)
{
	float inprev = 0;
	float outprev = 0;
	float coeff = tanf(REC_PI * 150.0f / 48000.0f);

	for (int i = 0; i < rec.bufpos; ++i)
	{
		float in = rec.recbuf[i];
		float out;

		out = in * coeff * 5.0f + inprev * coeff - (coeff - 1) * outprev;

		out /= (coeff + 1.33f);

		inprev = in;
		outprev = out;

		float mix = out * recQFX + in * (1 - recQFX);

		rec.recbuf[i] = mix;
	}
}

void RecorderHighBoost(Record& rec)
{
	float inprev = 0;
	float outprev = 0;
	float coeff = tanf(REC_PI * 18000.0f / 48000.0f);

	for (int i = 0; i < rec.bufpos; ++i)
	{
		float in = rec.recbuf[i];
		float out;

		out = in * 10.0f - inprev * coeff - (coeff - 1) * outprev;

		out /= (coeff + 1.33f);

		inprev = in;
		outprev = out;

		float mix = out * recQFX + in * (1 - recQFX);

		rec.recbuf[i] = mix;
	}
}

void RecorderSuperfi()
{
	if (undolevel < RECMAXUNDOLEVEL)
		prevrec[undolevel++] = baserec;
	else
		return;

	RIAAFilter inverseriaa, riaa;
	PrepareInverseRIAAFilter(inverseriaa, 68, 250, 2000);
	PrepareRIAAFilter(riaa, 68, 200, 2000);

	RecorderHighCut(baserec);

	HelperProcessInverseRIAAFilter(baserec, inverseriaa);

	RecorderHighBoost(baserec);

	HelperProcessRIAAFilter(baserec, riaa);
}