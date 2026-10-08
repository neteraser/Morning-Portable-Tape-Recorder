#pragma once

#define RECUSEBASSLIB

#ifdef RECUSEBASSLIB
#include "bass/bass.h"
#endif

#include "PortYTrack/portytrack.h"

#include <vector>
#include <string>

typedef bool (RecCallback)(const float* data_out, int length);

typedef bool (PlayCallback)(float* data_in, int length);

class RecAudioLibrary
{
public:
	RecAudioLibrary() {
		error = 0;
		curDevice = 0;
		curRecDevice = 0;
		//devices.reserve(32);
		//recDevices.reserve(32);
		playcall = 0;
		reccall = 0;
	}

	HWND window;
	DWORD error;

	std::vector<std::string> devices;
	std::vector<std::string> recDevices;
	int curDevice;
	int curRecDevice;
	RecCallback* reccall;
	PlayCallback* playcall;

	virtual bool Init(HWND, int) { return false; }
	virtual bool Rec() { return false; }
	virtual bool Play() { return false; }
	virtual bool Remaster() { return false; }
	virtual bool Stop() { return false; }
	virtual bool SetDevice(int) { return false; }
	virtual bool SetRecDevice(int) { return false; }
	virtual bool Retrieve(int, int) { return false; }
	virtual bool EnumerateDevices() { return false;  }
	virtual bool Free() { return false; }

	virtual bool SetCallbacks(RecCallback* rc, PlayCallback* pc) {
		reccall = rc;
		playcall = pc;
		return true;
	}

	virtual int GetDevice() { return curDevice;  }

	virtual int GetRecDevice() { return curRecDevice;  }

	virtual DWORD GetError() { return error;  }
};

#ifdef RECUSEBASSLIB
class BASSLibraryWrapper : public RecAudioLibrary {
public:
	HRECORD record;
	HSTREAM stream;

	virtual bool Init(HWND wnd, int basicdelayms);
	virtual bool Rec();
	virtual bool Play();
	virtual bool Remaster();
	virtual bool Stop();
	virtual bool SetDevice(int devNum);
	virtual bool SetRecDevice(int recDevNum);
	virtual bool Retrieve(int devNum, int recDevNum);
	virtual bool Free();
	virtual int GetDevice();
	virtual int GetRecDevice();
	virtual bool EnumerateDevices();
	virtual DWORD GetError();
};
#endif 

class PortYTrackLibraryWrapper : public RecAudioLibrary {
public:
	virtual bool Init(HWND wnd, int basicdelayms);
	virtual bool Rec();
	virtual bool Play();
	virtual bool Remaster();
	virtual bool Stop();
	virtual bool SetDevice(int devNum);
	virtual bool SetRecDevice(int recDevNum);
	//virtual bool Retrieve(int devNum, int recDevNum);
	virtual bool Free();
	/*virtual int GetDevice();
	virtual int GetRecDevice();	*/
	virtual bool EnumerateDevices();

	virtual bool SetCallbacks(RecCallback* rc, PlayCallback* pc);
};

#define AUDIOLIBNUMBER_PORTYTRACK 0
#define AUDIOLIBNUMBER_BASS 1

bool SwitchAudioLibrary(int audiolibrarynumber, HWND wnd, int delay);

extern RecAudioLibrary* gaudio;