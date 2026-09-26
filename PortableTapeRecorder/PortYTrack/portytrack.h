#pragma once

#include <Windows.h>
#include <string>
#include <vector>

#ifndef PORTYTRACKDLL
#define PORTYTRACKDLL __declspec(dllimport)
#endif

PORTYTRACKDLL bool PortYTrackInit(HWND wnd);
 
PORTYTRACKDLL bool PortYTrackRec();

PORTYTRACKDLL bool PortYTrackPlay();

PORTYTRACKDLL bool PortYTrackRemaster();

PORTYTRACKDLL bool PortYTrackStop();

typedef bool (PortYTrackRecCallback)(const float* data_out, int length);

typedef bool (PortYTrackPlayCallback)(float* data_in, int length);

PORTYTRACKDLL void PortYTrackSetCallbacks(PortYTrackRecCallback* reccall, PortYTrackPlayCallback* playcall);

PORTYTRACKDLL bool PortYTrackUpdate();

PORTYTRACKDLL bool PortYTrackRetrieve();

PORTYTRACKDLL bool PortYTrackFree();

struct PortYTrackDeviceList {
	std::vector<std::string> devices;
	std::vector<std::string> recDevices;
};

PORTYTRACKDLL bool PortYTrackEnumerateDevices(PortYTrackDeviceList& devicelist);

PORTYTRACKDLL bool PortYTrackSetRecDevice(int i);

PORTYTRACKDLL bool PortYTrackSetPlayDevice(int i);