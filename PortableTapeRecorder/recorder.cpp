
#include "recorder.h"
#include "log.h"
#include "audiofile.h"
#include "lame/lame.h"
#include <immintrin.h>
#include <direct.h>

Record baserec;
Record streamrec;
//int recOrStop = 1;
//int playOrStop = 1;

extern int tapeSlowdownSamples;
extern int adcIntMaxNatural;

std::string gfilename;

void InitFolders()
{
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

float CatmullRom1D(float t, float p0, float p1, float p2, float p3)
{
	float t2 = t * t;
	float t3 = t2 * t;
	return (0.5f * ((2.0f * p1) + (-p0 + p2) * t +
		(2.0f * p0 - 5.0f * p1 + 4 * p2 - p3) * t2 +
		(-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3));
}

int recOrPlay= 0;

bool noRecDevice = false;

float recDeviceVolume = 0.0f;
float recDeviceRecTime = 0.0;

float masterDevicePlayTime = 0.0f;

float recFadeInTime = 2.5f;

float FixDeviceBit(float in)
{
	float out = -in;
	out *= (float)(adcIntMaxNatural) / 32767.0f;
	if (out < 0.0f)
	{
		out *= 32768.0f / 32767.0f;
	}
	return -out;
}

BOOL CALLBACK record_proc(HRECORD handle,
	const void* buffer,
	DWORD length,
	void* user)
{
	_m_prefetchrs(baserec.recbuf + baserec.bufpos);
	_m_prefetchrs(buffer);

	if(remastermode != 0)
		recOrPlay = -1;

	Record* prec = &baserec;//(Record*)user;
	float* fbuf = (float*)buffer;

	if (prec->breakrec == TRUE)
	{
		return FALSE;
	}

//	if (timeGetTime() - prec->lastreccallback_time > 1000)
//		return FALSE;

	prec->lastreccallback_time = timeGetTime();

	if (!noRecDevice)
	{
		length = length / sizeof(float);

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
			__m256 fvi_intr = _mm256_load_ps(&fbuf[i]);

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
				return FALSE;
			}
		}
		for (int i = len8 * 8; i < lenrest; ++i)
		{
			float fvi = fbuf[i];
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
				return FALSE;
			}
		}
	}

	return TRUE;
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
	_m_prefetchrs(baserec.rectempbuf);

	baserec.temppos -= 256;

	int recslowdownsamples = RECSLOWDOWNSAMPLES;
	if (tapeSlowdownSamples > 0)
		recslowdownsamples = 256 + tapeSlowdownSamples;

	for (int i = 0; i < recslowdownsamples; ++i)
	{
		float src_idx = (float)i / (float)(recslowdownsamples) * 256.0f;
		int idx = (int)roundf(src_idx) + baserec.temppos;
		int idxprev = (int)roundf(src_idx - 1.0f) + baserec.temppos;
		int idxnext = (int)roundf(src_idx + 1.0f) + baserec.temppos;
		float spos = 0.5f + (float)(recslowdownsamples) / 256.0f - 1.0f;

		float vclean = baserec.rectempbuf[idx];

		float v1safe = 0;
		if (idx > 0)
			v1safe = baserec.rectempbuf[idxprev];
		else
			v1safe = proc256prevbuf;

		float v2safe = 0;
		if (idx - baserec.temppos + 1 < 256)
			v2safe = baserec.rectempbuf[idxnext];
		else
			v2safe = vclean;

		float fintrp = CalcSplineInterpolation(v1safe, vclean, v2safe, spos);

		float fv = vclean + fintrp * 0.1f;

		//fv += 0.16f;
		
		baserec.recbuf[baserec.bufpos++] = fv;
	}
	proc256prevbuf = baserec.rectempbuf[baserec.temppos + 255];
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


void PostProcessRec(Record& rec) {
	PrepareRIAAFilter(baseriaafilter, 30, 150, 2000);
	rec.peak = 0;
	rec.peakpos = 0;
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fsample = rec.recbuf[i];
		float ffilter = 0;
		ffilter = ProcessRIAAFilter(baseriaafilter, fsample);
		ffilter *= 0.166f;
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

	WriteToLog("Record started... ticktime: %i", timeGetTime());

	ZeroMemory(rec.recbuf, RECBUFSTANDARDSIZE * sizeof(float));
	rec.temppos = 0;
	rec.bufpos = 0;
	rec.writepos = 0;
	rec.breakrec = FALSE;
	rec.peak = 0.0f;
	rec.peakpos = 0;
	rec.lastreccallback_time = 0;

	PrepareInverseRIAAFilter(baseinvriaafilter, 68, 250, 2000);

	rec.tape = fopen(getProgramFileName("tape.bin"), "wb");
	rec.tapepolarized = fopen(getProgramFileName("tape_polarized.bin"), "wb");

	noRecDevice = false;

	baserec.record = BASS_RecordStart(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, record_proc, &baserec);

	if (baserec.record == 0)
	{
		WriteToLog("BASS RecordStart at StartRec failed");
	}

	//BASS_ChannelIsActive()

	
	BASS_RecordSetInput(-1, BASS_INPUT_ON, 1.0);
	
	/**/

	if (BASS_ChannelPlay(baserec.record, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelStart at StartRec failed");
	}

	DWORD error = BASS_ErrorGetCode();
	if (error != BASS_OK)
	{
		WriteToLog("BASS error at StartRec, error code: %i", error);
	}
	recOrPlay = -1;
}


Record prevrec[(RECMAXUNDOLEVEL * 2)];

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
		friaa *= 0.066f;
		fsample = (fsample + friaa) / 1.66f;
		CheckRecorderPeak(fsample, i);
		rec.recbuf[i] = fsample;
	}
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
	for (int i = 0; i < rec.bufpos; ++i)
	{
		float fv = rec.recbuf[i];		
		fv *= fmult;
		rec.recbuf[i] = fv;
	}
	rec.peak = 0.999f;
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
			fmult2 = (fabs(fv) - limiter_peak) * 0.2f;
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
			float fv, fvp;
			fv = -PolarizeFloat(Int16ToFloat(UnpackInt16ForReading(sv[i])), false);
			if (baserec.tapepolarized != 0)
			{
				fvp = PolarizeFloat(Int16ToFloat(UnpackInt16ForReading(svp[i])), true);
			}
			else
			{
				fvp = fv;
			}
			float fvv = baserec.recbuf[readpos];
			baserec.recbuf[readpos] = (fv * 0.7f + fvp * 0.3f) - fvv * 0.1f;
			readpos += 1;
		}
	}
	fclose(baserec.tape);
	if(baserec.tapepolarized != 0)
		fclose(baserec.tapepolarized);
}

void StopRec(Record& rec)
{
	if (recOrPlay >= 0)
		return;

	WriteToLog("Record stopped... ticktime: %i", timeGetTime());

	rec.breakrec = FALSE;

	remastermode = 1;

	recOrPlay = 0;

	recOrStop = 1;

	fclose(rec.tape);
	fclose(rec.tapepolarized);

	if (BASS_ChannelStop(rec.record) != TRUE)
	{
		WriteToLog("BASS ChannelStop at StopRec failed");
	}
	/*if (BASS_StreamFree(rec.record) != TRUE)
	{
		WriteToLog("BASS StreamFree at StopRec failed");
	}*/
	rec.record = 0;

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
	if ( BASS_ChannelStop(streamrec.stream) != TRUE )
	{
		WriteToLog("BASS ChannelStop at StopPlayingRec failed");
	}
	if (BASS_StreamFree(streamrec.stream) != TRUE)
	{
		WriteToLog("BASS StreamFree at StopPlayingRec failed");
	}
	streamrec.stream = 0;
}

DWORD CALLBACK play_proc(HSTREAM handle,
	void* buffer,
	DWORD length,
	void* user)
{
	if(remastermode != 0)
		recOrPlay = 1;

	if (streamrec.breakplay == TRUE)
	{
		return BASS_STREAMPROC_END;
	}

	streamrec.playcallback_dt = timeGetTime() - streamrec.lastplaycallback_time;
	streamrec.lastplaycallback_time = timeGetTime();

	float* fbuf = (float*)buffer;
	unsigned ilength = length / sizeof(float);
	for (int i = 0; i < ilength; ++i)
	{
		fbuf[i] = streamrec.recbuf[streamrec.playpos++];
		//if (baserec.playpos >= RECBUFSTANDARDSIZE)
		//	return BASS_STREAMPROC_END;
		if (streamrec.playpos >= streamrec.bufpos + 4800)
		{
			if(playloop && remastermode != 0)
			{
				streamrec.playpos = 0;
			}
			else
			{
				streamrec.breakplay = TRUE;
				return BASS_STREAMPROC_END;
			}
		}
	}
	return length;
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
	streamrec.lastplaycallback_time = 0;
	streamrec.playcallback_dt = 4800;
	streamrec.stream = BASS_StreamCreate(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, play_proc, 0);
	if (streamrec.stream == 0)
	{
		WriteToLog("BASS StreamCreate at PlayRec failed");
	}
	if (BASS_ChannelPlay(streamrec.stream, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelPlay at PlayRec failed");
	}
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

		MessageBox(0, savemessage, "Saved!", MB_ICONINFORMATION | MB_OK);
	}
	else
	{
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

		MessageBox(0, savemessage, "Saved!", MB_ICONINFORMATION | MB_OK);
	}

	if (!dontseek)
		fseek(counterfile, -4, SEEK_END);

	fwrite(&reccount, 4, 1, counterfile);

	fflush(counterfile);

	fclose(counterfile);

}

void PreprocessRec(Record& rec)
{
	static float previn = 0, prevout = 0;

	if (rec.writepos == 0)
	{
		previn = 0;
		prevout = 0;
	}
	for (int i = rec.writepos; i < rec.bufpos; ++i)
	{
		float fv = rec.recbuf[i];
		float out = (fv - previn) + 0.99f * prevout;
		rec.recbuf[i] = out;
		previn = fv;
		prevout = out;
	}
}


#define RECWRITECHUNK 128
#define RECREWRITECOUNT 3

//int recTapeRewrites = RECREWRITECOUNT;

void WriteRec()
{
	if (IsRecorderWriting())
	{
		PreprocessRec(baserec);

		while (baserec.writepos < baserec.bufpos)
		{
			int wlen = RECWRITECHUNK;
			int wdif = baserec.bufpos - baserec.writepos;
			if (wdif < wlen)
				wlen = wdif;

			float fv[RECWRITECHUNK];

			short sv[RECREWRITECOUNT][RECWRITECHUNK];
			short svp[RECREWRITECOUNT][RECWRITECHUNK];

			for (int i = 0; i < wlen; ++i)
			{
				float fvi = baserec.recbuf[baserec.writepos + i];
				fvi = fvi;

				float fveff = ProcessInverseRIAAFilter(baseinvriaafilter, fvi);
				fveff *= 0.0166f;

				fvi = (fvi + fveff);

				fvi += 0.001f;

				fv[i] = fvi;

				for (int j = 0; j < RECREWRITECOUNT; ++j)
				{
					sv[j][i] = PackInt16ForWriting(FloatToInt16(PolarizeFloat(-fvi, false)));
					svp[j][i] = PackInt16ForWriting(FloatToInt16(PolarizeFloat(fvi, true)));
					fvi -= 0.000166f;
				}
			}

			// standard 

			fwrite(sv[0], 2, wlen, baserec.tape);

			for (int i = 1; i < RECREWRITECOUNT; ++i)
			{
				fseek(baserec.tape, -wlen * 2, SEEK_CUR);
				fwrite(sv[i], 2, wlen, baserec.tape);
			}
			fflush(baserec.tape);

			// polarized 

			if (baserec.tapepolarized != 0)
			{

				fwrite(&svp[0], 2, wlen, baserec.tapepolarized);
				//baserec.writepos += wlen;

				for (int i = 1; i < RECREWRITECOUNT; ++i)
				{
					fseek(baserec.tapepolarized, -wlen * 2, SEEK_CUR);
					fwrite(&svp[i], 2, wlen, baserec.tapepolarized);
				}
				fflush(baserec.tapepolarized);

				baserec.writepos += wlen;
			}

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
			if (timeGetTime() - baserec.lastreccallback_time > 2000)
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
			}
		}
	}

	if (remastermode == 0)
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
					baserec.recbuf[baserec.bufpos++] = fv;
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
	WriteToLog("Remaster started..., timeticks: %i", timeGetTime());


	remastermode = 0;


	// play streamrec
	streamrec = baserec;

	//CopyMemory(streamrec.recbuf, baserec.recbuf, sizeof(baserec.recbuf));

	ZeroMemory(streamrec.recbuf, 48000 * sizeof(float));
	memcpy(streamrec.recbuf + 48000, baserec.recbuf, baserec.bufpos * sizeof(float));
	ZeroMemory(streamrec.recbuf + baserec.bufpos + 48000, 48000 * sizeof(float));
	streamrec.bufpos += 96000;

	ZeroMemory(baserec.recbuf, RECBUFSTANDARDSIZE * sizeof(float));
	
	streamrec.breakplay = FALSE;
	streamrec.playpos = 0;
	baserec.unipos = 0;

	streamrec.lastplaycallback_time = 0;
	streamrec.playcallback_dt = 4800;
	
	streamrec.stream = BASS_StreamCreate(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, play_proc, 0);

	if ( streamrec.stream == 0)
	{
		WriteToLog("BASS StreamCreate at StartRemaster failed");
	}

	if (BASS_ChannelPlay(streamrec.stream, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelPlay at StartRemaster failed");
	}

	//////////////////////////
	// rec baserec
	
	baserec.bufpos = 0;
	baserec.unipos = 0;
	baserec.playpos = 0;
	baserec.writepos = 0;
	baserec.breakrec = FALSE;
	baserec.peak = 0.0f;
	baserec.peakpos = 0;
	baserec.lastreccallback_time = 0;

	PrepareInverseRIAAFilter(baseinvriaafilter, 68, 250, 2000);

	baserec.tape = fopen(getProgramFileName("tape_remaster.bin"), "wb");

	noRecDevice = false;

	//BASS_RecordFree();

	//BASS_RecordInit(-1);

	baserec.record = BASS_RecordStart(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, record_proc, &baserec);

	if (baserec.record == 0)
	{
		WriteToLog("BASS RecordStart at StartRemaster failed");
	}

	//BASS_RecordSetInput(-1, BASS_INPUT_ON, 1.0);

	if (BASS_ChannelPlay(baserec.record, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelPlay at StartRemaster failed");
	}

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

	if (BASS_ChannelStop(baserec.record) != TRUE)
	{
		WriteToLog("BASS ChannelStop at StopRemaster failed");
	}

	/*if (BASS_StreamFree(baserec.record) != TRUE)
	{
		WriteToLog("BASS StreamFree at StopRemaster failed");
	}*/

	baserec.record = 0;

	ReadRec();

	PostProcessRec(baserec);

	// stop playing
	streamrec.playpos = 0;
	baserec.unipos = 0;
	streamrec.breakplay = TRUE;
	if (BASS_ChannelStop(streamrec.stream) != TRUE)
	{
		WriteToLog("BASS ChannelStop at StopRemaster failed");
	}
	if (BASS_StreamFree(streamrec.stream) != TRUE)
	{
		WriteToLog("BASS StreamFree at StopRemaster failed");
	}
	streamrec.stream = 0;

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
	else
	{
		return recOrPlay == 0;
	}
}

bool IsRecorderRemastering() {
	return remastermode == 0;
}

bool RecorderDeviceRetrieve(int devNum, int recDevNum)
{
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

	bool result = true;

	if (baserec.record != 0)
	{
		if (BASS_ChannelStop(baserec.record) != TRUE)
		{
			WriteToLog("BASS StreamFree at RecorderDeviceRetrieve failed");
			result = false;
		}
		baserec.record = 0;
	}

	if(streamrec.stream != 0)
	{
		if (BASS_StreamFree(streamrec.stream) != TRUE)
		{
			WriteToLog("BASS StreamFree at RecorderDeviceRetrieve failed");
			result = false;
		}
		streamrec.stream = 0;
	}

	if (BASS_RecordFree() != TRUE)
	{
		WriteToLog("BASS RecordFree at RecorderDeviceRetrieve failed");
		result = false;
	}

	if (BASS_Free() != TRUE)
	{
		WriteToLog("BASS Free at RecorderDeviceRetrieve failed");
		result = false;
	}
	else
		result = true;

	if (!result)
		return false;

	if (BASS_Init(devNum, 48000, BASS_DEVICE_MONO, mainWnd, 0) != TRUE)
	{
//		return false;
		WriteToLog("BASS Init at RecorderDeviceRetrieve failed");
	}

	BASS_Start();

	if (BASS_RecordInit(recDevNum) != TRUE)
	{
		WriteToLog("BASS RecordInit at RecorderDeviceRetrieve failed");
//		return false;
	}

	//BASS_SetConfig(BASS_CONFIG_BUFFER, 10);

	if (BASS_SetConfig(BASS_CONFIG_REC_BUFFER, 10) != TRUE)
	{
		WriteToLog("BASS SetConfig, REC_BUFFER at RecorderDeviceRetrieve failed");
//		return false;
	}
	/*
	BASS_SetDevice(0);
	BASS_SetDevice(devNum);

	BASS_RecordSetDevice(0);
	BASS_RecordSetDevice(recDevNum);*/
	DWORD error = BASS_ErrorGetCode();
	if (error != BASS_OK)
	{
		WriteToLog("BASS error at RecorderDeviceRetrieve: %i", error);
		return false;
	}
	return true;
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