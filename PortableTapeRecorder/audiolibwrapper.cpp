
#include <Windows.h>
#include "audiolibwrapper.h"
#include "recorder.h"
#include "log.h"

bool PortYTrackLibraryWrapper::Init(HWND myWnd, int)
{
	if (!PortYTrackInit(myWnd)) {
		WriteToLog("PortYTrack Init failed.");
		return false;
	}

	curDevice = 0;
	curRecDevice = 0;

	return true;
}

bool PortYTrackLibraryWrapper::Rec() 
{
	if (!PortYTrackRec())
	{
		WriteToLog("PortYTrack Rec failed.");
		return false;
	}

	return true;
}

bool PortYTrackLibraryWrapper::Play()
{
	if (!PortYTrackPlay())
	{
		WriteToLog("PortYTrack Play failed.");
		return false;
	}

	return true;
}

bool PortYTrackLibraryWrapper::Remaster()
{
	if (!PortYTrackRemaster()) {
		WriteToLog("PortYTrack Remaster failed");
		return false;
	}
	return true;

}

bool PortYTrackLibraryWrapper::Stop()
{
	if (!PortYTrackStop())
	{
		WriteToLog("PortYTrack Stop failed.");
		return false;
	}

	return true;
}

bool PortYTrackLibraryWrapper::SetCallbacks(RecCallback* rc, PlayCallback* pc)
{
	reccall = rc;
	playcall = pc;
	PortYTrackSetCallbacks(reccall, playcall);
	return true;
}

PortYTrackDeviceList portydevlist;

bool PortYTrackLibraryWrapper::EnumerateDevices()
{
	if (!PortYTrackEnumerateDevices(portydevlist))
	{
		WriteToLog("PortYTrack Device enumeration failed.");
		return false;
	}
	devices = portydevlist.devices;
	recDevices = portydevlist.recDevices;
	return true;
}

bool PortYTrackLibraryWrapper::SetDevice(int devn) 
{
	if (devn < 0)
		devn = 0;
	if (!PortYTrackSetPlayDevice(devn))
	{
		WriteToLog("PortYTrack SetDevice failed.");
		return false;
	}
	curDevice = devn;
	return true;
}

bool PortYTrackLibraryWrapper::SetRecDevice(int devn)
{
	if (devn < 0)
		devn = 0;

	if (!PortYTrackSetRecDevice(devn))
	{
		WriteToLog("PortYTrack SetRecDevice failed.");
		return false;
	}
	curRecDevice = devn;
	return true;
}

bool PortYTrackLibraryWrapper::Free()
{
	if (!PortYTrackFree())
	{
		WriteToLog("PortYTrack Free failed.");
		return false;
	}

	return true;
}
#ifdef RECUSEBASSLIB
bool BASSLibraryWrapper::Init(HWND myWnd, int basicdelayms)
{
	window = myWnd;

	if (!BASS_Init(-1, 48000, BASS_DEVICE_MONO, myWnd, 0))
	{
		WriteToLog("BASS Init failed.");
		return false;
	}
	if (!BASS_RecordInit(-1))
	{
		WriteToLog("BASS RecordInit failed.");
		return false;
	}

	if (!BASS_SetConfig(BASS_CONFIG_REC_BUFFER, basicdelayms))
	{
		WriteToLog("BASS SetConfig failed.");
		return false;
	}

	return true;
}

bool BASSLibraryWrapper::Free() 
{
	if (!BASS_RecordFree())
	{
		WriteToLog("BASS RecordFree failed.");
		return false;
	}

	if (!BASS_Free())
	{
		WriteToLog("BASS Free failed.");
		return false;
	}

	return true;
}

int BASSLibraryWrapper::GetDevice()
{
	return BASS_GetDevice();
}

int BASSLibraryWrapper::GetRecDevice()
{
	return BASS_RecordGetDevice();
}

bool BASSLibraryWrapper::EnumerateDevices()
{
	devices.clear();
	recDevices.clear();

	BOOL nextdevice = TRUE;
	DWORD bassdevi = 0;
	BASS_DEVICEINFO devinfo;
	while (nextdevice) {
		nextdevice = BASS_GetDeviceInfo(bassdevi, &devinfo);
		if (!nextdevice)
			break;
		if (devinfo.flags & BASS_DEVICE_ENABLED)
		{
			std::string newname(devinfo.name);
			devices.push_back(newname);
		}
		bassdevi += 1;
	}

	bassdevi = 0;
	nextdevice = TRUE;

	while (nextdevice) {
		nextdevice = BASS_RecordGetDeviceInfo(bassdevi, &devinfo);
		if (!nextdevice)
			break;
		if (devinfo.flags & BASS_DEVICE_ENABLED)
		{
			std::string newname(devinfo.name);
			recDevices.push_back(newname);
		}
		bassdevi += 1;
	}
	return true;
}

bool BASSLibraryWrapper::SetDevice(int devn)
{
	if (!BASS_SetDevice(devn))
	{
		WriteToLog("BASS SetDevice failed.");
		return false;
	}

	return true;
}

bool BASSLibraryWrapper::SetRecDevice(int devn)
{
	if (!BASS_RecordSetDevice(devn))
	{
		WriteToLog("BASS RecordSetDevice failed.");
		return false;
	}
	return true;
}

BOOL CALLBACK bass_record_proc(HRECORD handle,
	const void* buffer,
	DWORD length,
	void* user)
{
	if (!gaudio->reccall((const float*)buffer, length / sizeof(float)))
		return FALSE;
	return TRUE;
}

DWORD CALLBACK bass_play_proc(HSTREAM handle,
	void* buffer,
	DWORD length,
	void* user)
{
	if (!gaudio->playcall((float*)buffer, length / sizeof(float)))
		return BASS_STREAMPROC_END;
	return length;
}

bool BASSLibraryWrapper::Rec()
{
	record = BASS_RecordStart(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, bass_record_proc, 0);

	if (record == 0)
	{
		WriteToLog("BASS RecordStart at Rec failed");
		return false;
	}

	/*if (!BASS_RecordSetInput(-1, BASS_INPUT_ON, 1.0))
	{
		WriteToLog("BASS RecordSetInput failed.");
	}*/

	if (BASS_ChannelPlay(record, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelStart at Rec failed");
		return false;
	}

	DWORD error = BASS_ErrorGetCode();
	if (error != BASS_OK)
	{
		WriteToLog("BASS error at Rec, error code: %i", error);
		return false;
	}

	return true;
}

bool BASSLibraryWrapper::Play()
{
	stream = BASS_StreamCreate(RECSAMPLERATE, 1, BASS_SAMPLE_FLOAT, bass_play_proc, 0);
	if (stream == 0)
	{
		WriteToLog("BASS StreamCreate at PlayRec failed");
		return false;
	}
	if (BASS_ChannelPlay(stream, TRUE) != TRUE)
	{
		WriteToLog("BASS ChannelPlay at PlayRec failed");
		return false;
	}
	return true;
}

bool BASSLibraryWrapper::Remaster()
{
	if (!Rec())
		return false;
	if (!Play())
		return false;
	return true;
}

bool BASSLibraryWrapper::Stop()
{
	if (record != 0)
	{
		if (BASS_ChannelStop(record) != TRUE)
		{
			WriteToLog("BASS ChannelStop at StopRec failed");
		}
		record = 0;
	}

	if (stream != 0)
	{
		if (BASS_ChannelStop(stream) != TRUE)
		{
			WriteToLog("BASS ChannelStop at StopRec failed");
		}
		if (BASS_StreamFree(stream) != TRUE)
		{
			WriteToLog("BASS StreamFree at StopPlayingRec failed");
		}
		stream = 0;
	}
	return true;
}

DWORD BASSLibraryWrapper::GetError()
{
	error = BASS_ErrorGetCode();
	return error;
}

bool BASSLibraryWrapper::Retrieve(int devNum, int recDevNum)
{
	bool result = true;

	if (record != 0)
	{
		if (BASS_ChannelStop(record) != TRUE)
		{
			WriteToLog("BASS StreamFree at RecorderDeviceRetrieve failed");
			result = false;
		}
		record = 0;
	}

	if (stream != 0)
	{
		if (BASS_StreamFree(stream) != TRUE)
		{
			WriteToLog("BASS StreamFree at RecorderDeviceRetrieve failed");
			result = false;
		}
		stream = 0;
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

	if (BASS_Init(devNum, 48000, BASS_DEVICE_MONO, window, 0) != TRUE)
	{
		//		return false;
		WriteToLog("BASS Init at DeviceRetrieve failed");
	}

	if (!BASS_Start())
	{
		WriteToLog("BASS Start at DeviceRetrieve failed");
	}

	if (BASS_RecordInit(recDevNum) != TRUE)
	{
		WriteToLog("BASS RecordInit at DeviceRetrieve failed");
		//		return false;
	}

	//BASS_SetConfig(BASS_CONFIG_BUFFER, 10);

	/*if (!BASS_SetConfig(BASS_CONFIG_BUFFER, 250))
	{
		WriteToLog("BASS SetConfig, BUFFER at RecorderDeviceRetrieve failed.");
	}*/

	if (BASS_SetConfig(BASS_CONFIG_REC_BUFFER, RECBUFFERDELAYMS) != TRUE)
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
#endif // RECUSEBASSLIB

#define MAXAUDIOLIBRARIES 4
std::string audiolibrarynames[MAXAUDIOLIBRARIES] = { "PortYTrack Library", "BASS Library" };

#ifdef RECUSEBASSLIB
BASSLibraryWrapper bassAudio;
#endif

PortYTrackLibraryWrapper portytrackAudio;

RecAudioLibrary* gaudio = &portytrackAudio;

bool SwitchAudioLibrary(int audiolibrarynumber, HWND wnd, int delay)
{
	if (audiolibrarynumber < 0)
		return false;
	if (audiolibrarynumber >= MAXAUDIOLIBRARIES)
		return false;

	WriteToLog("Switching audio library: using %s", audiolibrarynames[audiolibrarynumber].c_str());
	if (!gaudio->Free())
	{
		WriteToLog("Audio library Free at switching libraries failed");
		return false;
	}
	switch (audiolibrarynumber)
	{
	case AUDIOLIBNUMBER_PORTYTRACK:
		gaudio = &portytrackAudio;
		break;
#ifdef RECUSEBASSLIB
	case AUDIOLIBNUMBER_BASS:
		gaudio = &bassAudio;
		break;
#else
	case AUDIOLIBNUMBER_BASS:
		WriteToLog("BASS Library was switched off in this build.");
#endif
	default:
		gaudio = &portytrackAudio;
	}
	
	Sleep(500);

	if (!gaudio->Init(wnd, delay))
	{
		WriteToLog("Audio Library Init failed at switching audio libraries");
		return false;
	}

	gaudio->EnumerateDevices();

	Sleep(500);

	return true;
}