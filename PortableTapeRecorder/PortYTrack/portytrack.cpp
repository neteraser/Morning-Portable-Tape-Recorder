
#define PORTYTRACKDLL __declspec(dllexport)

#include "portytrack.h"
#include <dsound.h>
//#include <combaseapi.h>
#include <process.h>
#include <thread>
#include <chrono>

HWND gwnd;

LPDIRECTSOUND8 dsound = 0;
LPDIRECTSOUNDCAPTURE8 dcapture = 0;
LPDIRECTSOUNDCAPTUREBUFFER dCaptureBuffer = 0;
LPDIRECTSOUNDBUFFER dBuffer = 0;
LPDIRECTSOUNDFULLDUPLEX dfullduplex = 0;
LPDIRECTSOUNDBUFFER8 dBuffer8 = 0;
LPDIRECTSOUNDCAPTUREBUFFER8 dCaptureBuffer8 = 0;


PortYTrackRecCallback* reccall = 0;
PortYTrackPlayCallback* playcall = 0;

bool isPlaying = false;
bool isRecording = false;
bool isRemastering = false;

struct PortYTrackDevice
{
    std::string devname;
    LPGUID deviceId;
};

PortYTrackDevice devices[128];
PortYTrackDevice recDevices[128];
int devicesNumber = 0;
int recDevicesNumber = 0;
int deviceNumber = 0;
int recDeviceNumber = 0;

long lastRecPos = 0;
long lastUpdatePos = 0;


#define PLAYUPDATECHUNK 4096
#define RECUPDATECHUNK 1024

float portytrackbuf[PLAYUPDATECHUNK * 4];

float portytrackplaybuf[PLAYUPDATECHUNK * 4];

__inline float PortYInt16ToFloat(short sv)
{
    sv -= 1;
    float fv = ((float)sv);
    //fv *= 32768.0f / 32767.0f;
    fv /= 32768.0f;
    return fv;
}

__inline float PortYInt8ToFloat(char cv)
{
    float fv = ((float)cv);
    //fv *= 32768.0f / 32767.0f;
    fv /= 256.0f;
    return fv;
}

__inline short PortYFloatToInt16(float fv)
{
    short sv;
    if (fv > 1.0)
        fv = 1.0f;
    if (fv < -1.0)
        fv = -1.0f;
    if (fv > 0.0f)
        sv = (short)(fv * 32766.0f);
    else
        sv = (short)(fv * 32767.0f);
    return sv;
}

__inline char PortYFloatToInt8(float fv)
{
    char cv;
    if (fv > 1.0)
        fv = 1.0f;
    if (fv < -1.0)
        fv = -1.0f;
    if (fv > 0.0f)
        cv = (char)(fv * 255.0f);
    else
        cv = (char)(fv * 256.0f);
    return cv;
}

bool autoUpdate = false;

void PortYUpdateThread(void*)
{
    while (true)
    {
        if (autoUpdate)
        {
            autoUpdate = false;

            if (!PortYTrackUpdate())
            {
                //MessageBox(gwnd, "PortYTrack Update failed", "Error", MB_OK | MB_ICONERROR);
            }
            autoUpdate = true;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(66));
    }
    _endthread();
}

bool PortYTrackInit(HWND wnd)
{
    gwnd = wnd;
/*HRESULT hr = CoCreateInstance(&CLSID_DirectSound8,
        NULL,
        CLSCTX_INPROC_SERVER,
        IID_IDirectSound8,
        (LPVOID*)&dsound);
*/
    HRESULT hr;
    
    hr = DirectSoundCreate8(0, &dsound, NULL);

    if (FAILED(hr))
    {
        return false;
    }

    /*hr = dsound->Initialize(NULL);
    if (FAILED(hr))
    {
        return false;
    }*/

    hr = dsound->SetCooperativeLevel(wnd, DSSCL_PRIORITY);
    if (FAILED(hr))
    {
        return false;
    }

    hr = DirectSoundCaptureCreate8(0, &dcapture, 0);
    if (FAILED(hr))
    {
        return false;
    }

    /*hr = dcapture->Initialize(0);
    if (FAILED(hr))
    {
        return false;
    }*/

    autoUpdate = true;
    _beginthread(PortYUpdateThread, 0, NULL);

    return true;
}

bool PortYTrackRec()
{
    if (isRecording)
        return false;

    HRESULT hr;

    if (dCaptureBuffer != 0)
    {
        return false;
    }

    DSCBUFFERDESC dBufferDesc;

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 48000;
    wfx.nAvgBytesPerSec = 96000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.cbSize = 0;

    dBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
    dBufferDesc.dwFlags = 0;
    dBufferDesc.dwBufferBytes = 48000 * 250 * 2;
    dBufferDesc.dwReserved = 0;
    dBufferDesc.lpwfxFormat = &wfx;
    dBufferDesc.dwFXCount = 0;
    dBufferDesc.dwFXCount = 0;
    dBufferDesc.lpDSCFXDesc = NULL;

    hr = dcapture->CreateCaptureBuffer(&dBufferDesc, &dCaptureBuffer, NULL);
    if (FAILED(hr))
    {
        //hr = pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)ppDSCB8);
        //pDSCB->Release();
        return false;
    }
    
    //Sleep(100);

    /*hr = dCaptureBuffer->Initialize(dcapture, &dBufferDesc);
    if (FAILED(hr))
    {
        return false;
    }*/

    hr = dCaptureBuffer->Start(NULL);
    if (FAILED(hr))
    {
        return false;
    }

    lastRecPos = 0;
    isRecording = true;

    autoUpdate = true;

    return true;
}

bool PortYTrackPlay()
{
    if (isPlaying)
        return false;

    if (dBuffer != 0)
    {
        return false;
    }

    lastUpdatePos = 0;

    HRESULT hr;


    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 48000;
    wfx.nAvgBytesPerSec = 96000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.cbSize = 0;

    DSBUFFERDESC dBufferDesc;
    dBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    dBufferDesc.dwFlags = DSBCAPS_CTRLVOLUME;
    dBufferDesc.dwBufferBytes = 48000 * 250 * 2; 
    dBufferDesc.dwReserved = 0;
    dBufferDesc.lpwfxFormat = &wfx;
    dBufferDesc.guid3DAlgorithm = GUID_NULL;

    //LPDIRECTSOUNDBUFFER tempBuffer;
    hr = dsound->CreateSoundBuffer(&dBufferDesc, & dBuffer, NULL);
    if (FAILED(hr))
        return false;

    /*hr = tempBuffer->QueryInterface(IID_IDirectSoundBuffer8, (LPVOID*)&dBuffer);
    if (FAILED(hr))
        return false;
    tempBuffer->Release();
    */
    /*
    hr = dBuffer->SetFormat(&wfx);
    if (FAILED(hr))
        return false;
        */
    LPVOID zerobuf = 0;
    DWORD zerobuflen = 0;

    hr = dBuffer->Lock(0, PLAYUPDATECHUNK * 2, &zerobuf, &zerobuflen, 0, 0, 0);
    if (FAILED(hr))
        return false;

    ZeroMemory(zerobuf, zerobuflen);

    hr = dBuffer->Unlock(zerobuf, zerobuflen, 0, 0);
    if (FAILED(hr))
        return false;

    lastUpdatePos += zerobuflen;

    hr = dBuffer->SetCurrentPosition(0);
    if (FAILED(hr))
        return false;

    hr = dBuffer->SetVolume(DSBVOLUME_MAX);
    if (FAILED(hr))
        return false;

    hr = dBuffer->Play(0, 0, NULL);
    if (FAILED(hr))
        return false;

    isPlaying = true;

    autoUpdate = true;

    return true;
}

bool PortYTrackRemaster()
{
    if (isRemastering)
        return false;

    if (dCaptureBuffer8 != 0)
        return false;

    if (dBuffer8 != 0)
        return false;

    if (dfullduplex != 0)
        return false;

    HRESULT hr;

    WAVEFORMATEX wfxRec;
    wfxRec.wFormatTag = WAVE_FORMAT_PCM;
    wfxRec.nChannels = 1;
    wfxRec.nSamplesPerSec = 48000;
    wfxRec.nAvgBytesPerSec = 96000;
    wfxRec.wBitsPerSample = 16;
    wfxRec.nBlockAlign = 2;
    wfxRec.cbSize = 0;

    WAVEFORMATEX wfx;
    wfx.wFormatTag = WAVE_FORMAT_PCM;
    wfx.nChannels = 1;
    wfx.nSamplesPerSec = 48000;
    wfx.nAvgBytesPerSec = 96000;
    wfx.wBitsPerSample = 16;
    wfx.nBlockAlign = 2;
    wfx.cbSize = 0;

    DSCBUFFERDESC dCaptureBufferDesc;

    dCaptureBufferDesc.dwSize = sizeof(DSCBUFFERDESC);
    dCaptureBufferDesc.dwFlags = 0;
    dCaptureBufferDesc.dwBufferBytes = 48000 * 250 * 2;
    dCaptureBufferDesc.dwReserved = 0;
    dCaptureBufferDesc.lpwfxFormat = &wfxRec;
    dCaptureBufferDesc.dwFXCount = 0;
    dCaptureBufferDesc.lpDSCFXDesc = NULL;

    DSBUFFERDESC dSoundBufferDesc;
    dSoundBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    dSoundBufferDesc.dwFlags = 0;
    dSoundBufferDesc.dwBufferBytes = 48000 * 250 * 2;
    dSoundBufferDesc.dwReserved = 0;
    dSoundBufferDesc.lpwfxFormat = &wfx;
    dSoundBufferDesc.guid3DAlgorithm = DS3DALG_DEFAULT;

    hr = DirectSoundFullDuplexCreate(recDevices[recDeviceNumber].deviceId, devices[deviceNumber].deviceId,
        &dCaptureBufferDesc, &dSoundBufferDesc, gwnd, DSSCL_PRIORITY, &dfullduplex, &dCaptureBuffer8, &dBuffer8,
        0);
    if (FAILED(hr))
    {
        return false;
    }

    //Sleep(100);

    hr = dCaptureBuffer8->Start(NULL);
    if (FAILED(hr))
    {
        return false;
    }

    lastRecPos = 0;

    //Sleep(100);

    lastUpdatePos = 0;
   
    LPVOID zerobuf = 0;
    DWORD zerobuflen = 0;

    hr = dBuffer8->Lock(0, PLAYUPDATECHUNK * 2, &zerobuf, &zerobuflen, 0, 0, 0);
    if (FAILED(hr))
        return false;

    ZeroMemory(zerobuf, zerobuflen);

    hr = dBuffer8->Unlock(zerobuf, zerobuflen, 0, 0);
    if (FAILED(hr))
        return false;

    lastUpdatePos += zerobuflen;

    hr = dBuffer8->SetCurrentPosition(0);
    if (FAILED(hr))
        return false;
    
    /*
    hr = dBuffer8->SetVolume(DSBVOLUME_MAX);
    if (FAILED(hr))
        return false;
        */

    hr = dBuffer8->Play(0, 0, NULL);
    if (FAILED(hr))
        return false;

    isRemastering = true;

    autoUpdate = true;

    return true;
}


bool PortYTrackStop()
{
    autoUpdate = false;

    if (isRecording)
    {
        if (dCaptureBuffer == 0)
            return false;

        HRESULT hr;
        hr = dCaptureBuffer->Stop();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        hr = dCaptureBuffer->Release();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        dCaptureBuffer = 0;
        isRecording = false;
    }

    if (isPlaying)
    {
        if (dBuffer == 0)
            return false;

        HRESULT hr;
        hr = dBuffer->Stop();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        hr = dBuffer->Release();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        dBuffer = 0;
        isPlaying = false;
        //lastUpdatePos = 0;
    }

    if (isRemastering)
    {
        if (dCaptureBuffer8 == 0)
            return false;
        if (dBuffer8 == 0)
            return false;
        if (dfullduplex == 0)
            return false;


        HRESULT hr;

        hr = dBuffer8->Stop();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        hr = dBuffer8->Release();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        hr = dCaptureBuffer8->Stop();
        if (FAILED(hr))
            return false;

        //Sleep(100);
        hr = dCaptureBuffer8->Release();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        hr = dfullduplex->Release();
        if (FAILED(hr))
            return false;

        //Sleep(100);

        dBuffer8 = 0;
        dCaptureBuffer8 = 0;
        dfullduplex = 0;

        isRemastering = false;
    }

    autoUpdate = false;

    return true;
}

void PortYTrackSetCallbacks(PortYTrackRecCallback* rc, PortYTrackPlayCallback* pc)
{
    reccall = rc;
    playcall = pc;
}

template<typename BufferInterface> bool RefreshRecFunc(BufferInterface* buffer)
{
    if (buffer == 0)
        return false;

    HRESULT hr;
    DWORD captureStatus;
    hr = buffer->GetStatus(& captureStatus);
    if (FAILED(hr))
        return false;

    if (captureStatus == DSCBSTATUS_CAPTURING)
    {
        redowrite:
        DWORD capturePos = 0, readPos = 0;
        hr = buffer->GetCurrentPosition(& capturePos, &readPos);
        if (FAILED(hr))
            return false;

        DWORD curPos = capturePos;

        int diff = capturePos - lastRecPos;

        if (diff >= RECUPDATECHUNK)
        {
            LPVOID pAudio;
            DWORD pLen;
            hr = buffer->Lock(lastRecPos, RECUPDATECHUNK, &pAudio, &pLen, 0, 0, 0);
            if (FAILED(hr))
                return false;

            // SEND TO THE APP

            int samplesNumber = 0;
            samplesNumber = pLen / 2;

            if (samplesNumber > 8192)
                samplesNumber = 8192;

            short* sv = (short*)pAudio;

            for (int i = 0, j = 0; i < samplesNumber; ++i)
            {
                portytrackbuf[i] = PortYInt16ToFloat(sv[i]);
                //portytrackbuf[j++] = PortYInt16ToFloat(sv[i]);
            }

            hr = buffer->Unlock(pAudio, pLen, 0, 0);
            if (FAILED(hr))
                return false;

            reccall(portytrackbuf, samplesNumber);

            //////////////////

            lastRecPos += pLen;
            goto redowrite;
        }
    }
    return true;
}

template<typename BufferInterface> bool RefreshPlayFunc(BufferInterface* buffer)
{
    if (buffer == 0)
        return false;

    HRESULT hr;
    DWORD playStatus;
    hr = buffer->GetStatus(&playStatus);
    if (FAILED(hr))
        return false;

    if (playStatus == DSBSTATUS_PLAYING)
    {
        DWORD playPos = 0, writePos = 0;
        hr = buffer->GetCurrentPosition(&playPos, &writePos);
        if (FAILED(hr))
            return false;

        long diff = lastUpdatePos - playPos;// % (48000 * 2);

        if (diff >= PLAYUPDATECHUNK && diff <= PLAYUPDATECHUNK * 2)
        {
            int samplesNumber = 0;
            samplesNumber = PLAYUPDATECHUNK;

            playcall(portytrackplaybuf, samplesNumber);

            LPVOID pAudio;
            DWORD pLen;
            hr = buffer->Lock(lastUpdatePos, PLAYUPDATECHUNK * 2, &pAudio, &pLen, 0, 0, 0);
            if (FAILED(hr))
                return false;

            // RECEIVE FROM THE APP
            samplesNumber = pLen / 2;

            if (samplesNumber > PLAYUPDATECHUNK)
                samplesNumber = PLAYUPDATECHUNK;

            short* sv = (short*)pAudio;

            for (int i = 0; i < samplesNumber; ++i)
            {
                sv[i] = PortYFloatToInt16(portytrackplaybuf[i]);
            }

            //////////////////

            hr = buffer->Unlock(pAudio, pLen, 0, 0);
            if (FAILED(hr))
                return false;

            lastUpdatePos += pLen;
        }
    }
    return true;
}


template<typename CaptureBufferInterface, typename BufferInterface> 
bool RefreshRemasterFunc(CaptureBufferInterface* captureBuffer, BufferInterface* buffer)
{
    if (captureBuffer == 0)
        return false;

    if (buffer == 0)
        return false;



    HRESULT hr;

    DWORD captureStatus;
    hr = captureBuffer->GetStatus(&captureStatus);
    if (FAILED(hr))
        return false;

    if (captureStatus == DSCBSTATUS_CAPTURING)
    {
    redowrite:
        DWORD capturePos = 0, readPos = 0;
        hr = captureBuffer->GetCurrentPosition(&capturePos, &readPos);
        if (FAILED(hr))
            return false;

        int diff = capturePos - lastRecPos;

        if (diff >= RECUPDATECHUNK)
        {
            LPVOID pAudio;
            DWORD pLen;
            hr = captureBuffer->Lock(lastRecPos, RECUPDATECHUNK, &pAudio, &pLen, 0, 0, 0);
            if (FAILED(hr))
                return false;

            // SEND TO THE APP
            int samplesNumber = 0;
            samplesNumber = pLen / 2;

            if (samplesNumber > 8192)
                samplesNumber = 8192;

            short* sv = (short*)pAudio;

            for (int i = 0, j = 0; i < samplesNumber; ++i)
            {
                portytrackbuf[i] = PortYInt16ToFloat(sv[i]);
            }

            //////////////////

            hr = captureBuffer->Unlock(pAudio, pLen, 0, 0);
            if (FAILED(hr))
                return false;

            reccall(portytrackbuf, samplesNumber);

            lastRecPos += pLen;
            goto redowrite;
        }
    }


    DWORD playStatus;
    hr = buffer->GetStatus(&playStatus);
    if (FAILED(hr))
        return false;

    if (playStatus == DSBSTATUS_PLAYING)
    {
        DWORD playPos = 0, writePos = 0;
        hr = buffer->GetCurrentPosition(&playPos, &writePos);
        if (FAILED(hr))
            return false;

        long diff = lastUpdatePos - playPos;// % (48000 * 2);

        if ((diff >= PLAYUPDATECHUNK) && (diff <= PLAYUPDATECHUNK * 2))
        {
            int samplesNumber = 0;

            samplesNumber = PLAYUPDATECHUNK;

            playcall(portytrackplaybuf, samplesNumber);

            LPVOID pAudio;
            DWORD pLen;
            hr = buffer->Lock(lastUpdatePos, PLAYUPDATECHUNK * 2, &pAudio, &pLen, 0, 0, 0);
            if (FAILED(hr))
                return false;

            // RECEIVE FROM THE APP
            samplesNumber = pLen / 2;

            if (samplesNumber > PLAYUPDATECHUNK)
                samplesNumber = PLAYUPDATECHUNK;

            short* сv = (short*)pAudio;

            for (int i = 0, j = 0; i < samplesNumber; ++i)
            {
                сv[i] = PortYFloatToInt16(portytrackplaybuf[i]);
            }

            //////////////////

            hr = buffer->Unlock(pAudio, pLen, 0, 0);
            if (FAILED(hr))
                return false;

            lastUpdatePos += pLen;
        }
    }

    return true;
}


bool PortYTrackUpdate()
{
    if (isRecording)
    {
        if (dCaptureBuffer != 0)
        {
            if (!RefreshRecFunc<IDirectSoundCaptureBuffer>(dCaptureBuffer))
            {
                return false;
            }
        }
    }
    else
    if (isPlaying)
    {
        if (dBuffer != 0)
        {
            if (!RefreshPlayFunc<IDirectSoundBuffer>(dBuffer))
            {
                return false;
            }
        }
    }
    else
    if (isRemastering)
    {
        if (dBuffer8 != 0 && dCaptureBuffer8 != 0)
        {
            if (!RefreshRemasterFunc<IDirectSoundCaptureBuffer8, IDirectSoundBuffer8>(dCaptureBuffer8, dBuffer8))
            {
                return false;
            }
        }
    }
    return true;
}



BOOL CALLBACK DirectSoundEnumDevice(LPGUID id,
    LPCSTR name,
    LPCSTR drivername,
    LPVOID)
{
    int n = devicesNumber;
    devices[n].deviceId = id;
    devices[n].devname = name; 
    devicesNumber += 1;
    return true;
}

BOOL CALLBACK DirectSoundEnumRecDevice(LPGUID id,
    LPCSTR name,
    LPCSTR drivername,
    LPVOID)
{
    int n = recDevicesNumber;
    recDevices[n].deviceId = id;
    recDevices[n].devname = name;
    recDevicesNumber += 1;
    return true;
}


bool PortYTrackEnumerateDevices(PortYTrackDeviceList& list)
{
    devicesNumber = 0;

    HRESULT hr;
    hr = DirectSoundEnumerateA(DirectSoundEnumDevice, 0);
    if (FAILED(hr))
        return false;

    recDevicesNumber = 0;

    hr = DirectSoundCaptureEnumerateA(DirectSoundEnumRecDevice, 0);
    if (FAILED(hr))
        return false;

    list.devices.clear();

    for (int i = 0; i < devicesNumber; ++i)
    {
        list.devices.push_back(devices[i].devname);
    }

    list.recDevices.clear();

    for (int i = 0; i < recDevicesNumber; ++i)
    {
        list.recDevices.push_back(recDevices[i].devname);
    }

    return true;
}

bool PortYTrackSetPlayDevice(int devnum)
{
    if (devnum == deviceNumber)
        return true;

    if (devnum < 0 || devnum >= devicesNumber)
        return false;

    if (isPlaying)
        return false;

    HRESULT hr;
    hr = dsound->Release();
    if (FAILED(hr))
        return false;

    hr = DirectSoundCreate8(devices[devnum].deviceId, &dsound, 0);
    if (FAILED(hr))  
        return false;

    hr = dsound->SetCooperativeLevel(gwnd, DSSCL_PRIORITY);
    if (FAILED(hr))
    {
        return false;
    }

    deviceNumber = devnum;
    
    return true;
}

bool PortYTrackSetRecDevice(int devnum)
{
    if (devnum == recDeviceNumber)
        return true;

    if (devnum < 0 || devnum >= recDevicesNumber)
        return false;

    if (isRecording)
        return false;

    HRESULT hr;

    hr = DirectSoundCaptureCreate8(recDevices[devnum].deviceId, &dcapture, 0);
    if (FAILED(hr))
    {
        return false;
    }

    recDeviceNumber = devnum;

    return true;
}

bool PortYTrackFree()
{
    autoUpdate = false;

    bool retval = true;

    if (isRecording || isPlaying || isRemastering)
        if (!PortYTrackStop())
            retval = false;

    HRESULT hr;
    if (dsound != 0)
    {
        hr = dsound->Release();
        if (FAILED(hr))
            retval = false;
    }

    if (dcapture != 0)
    {
        hr = dcapture->Release();
        if (FAILED(hr))
            retval = false;
    }

    if (dfullduplex != 0)
    {
        hr = dfullduplex->Release();
        if (FAILED(hr))
            retval = false;
    }

    return retval;
}

/*
BOOL CALLBACK DSEnumProc(LPGUID lpGUID,
    LPCTSTR lpszDesc,
    LPCTSTR lpszDrvName,
    LPVOID lpContext)
{
    HWND hCombo = (HWND)lpContext;
    LPGUID lpTemp = NULL;

    if (lpGUID != NULL)  //  NULL only for "Primary Sound Driver".
    {
        if ((lpTemp = (LPGUID)malloc(sizeof(GUID))) == NULL)
        {
            return(TRUE);
        }
        memcpy(lpTemp, lpGUID, sizeof(GUID));
    }

    ComboBox_AddString(hCombo, lpszDesc);
    ComboBox_SetItemData(hCombo,
        ComboBox_FindString(hCombo, 0, lpszDesc),
        lpTemp);
    free(lpTemp);
    return(TRUE);
}

if (FAILED(DirectSoundEnumerate((LPDSENUMCALLBACK)DSEnumProc,
    (VOID*)&hCombo)))
{
    EndDialog(hDlg, TRUE);
    return(TRUE);
}

HRESULT hr = CoInitializeEx(NULL, 0);
if (FAILED(hr))
{
    ErrorHandler(hr);  // Add error-handling here.
}



CoUninitialize();


BOOL AppCreateWritePrimaryBuffer(
    LPDIRECTSOUND8 lpDirectSound,
    LPDIRECTSOUNDBUFFER * lplpDsb,
    LPDWORD lpdwBufferSize,
    HWND hwnd)
{
    DSBUFFERDESC dsbdesc;
    DSBCAPS dsbcaps;
    HRESULT hr;
    WAVEFORMATEX wf;

    // Set up wave format structure. 
    memset(&wf, 0, sizeof(WAVEFORMATEX));
    wf.wFormatTag = WAVE_FORMAT_PCM;
    wf.nChannels = 2;
    wf.nSamplesPerSec = 22050;
    wf.nBlockAlign = 4;
    wf.nAvgBytesPerSec =
        wf.nSamplesPerSec * wf.nBlockAlign;
    wf.wBitsPerSample = 16;

    // Set up DSBUFFERDESC structure. 
    memset(&dsbdesc, 0, sizeof(DSBUFFERDESC));
    dsbdesc.dwSize = sizeof(DSBUFFERDESC);
    dsbdesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    // Buffer size is determined by sound hardware. 
    dsbdesc.dwBufferBytes = 0;
    dsbdesc.lpwfxFormat = NULL; // Must be NULL for primary buffers. 

    // Obtain write-primary cooperative level. 
    hr = lpDirectSound->SetCooperativeLevel(hwnd, DSSCL_WRITEPRIMARY);
    if SUCCEEDED(hr)
    {
        // Try to create buffer. 
        hr = lpDirectSound->CreateSoundBuffer(&dsbdesc,
            lplpDsb, NULL);
        if SUCCEEDED(hr)
        {
            // Set primary buffer to desired format. 
            hr = (*lplpDsb)->SetFormat(&wf);
            if SUCCEEDED(hr)
            {
                // If you want to know the buffer size, call GetCaps. 
                dsbcaps.dwSize = sizeof(DSBCAPS);
                (*lplpDsb)->GetCaps(&dsbcaps);
                *lpdwBufferSize = dsbcaps.dwBufferBytes;
                return TRUE;
            }
        }
    }
    // Failure. 
    *lplpDsb = NULL;
    *lpdwBufferSize = 0;
    return FALSE;
}

DWORD pv;  // Can be any 32-bit type.

HRESULT hr = DirectSoundCaptureEnumerate(
    (LPDSENUMCALLBACK)DSEnumProc, (VOID*)&pv);


HRESULT DirectSoundCaptureCreate8(
    LPCGUID lpcGUID,
    LPDIRECTSOUNDCAPTURE8 * lplpDSC,
    LPUNKNOWN pUnkOuter
)

HRESULT CreateCaptureBuffer(LPDIRECTSOUNDCAPTURE8 pDSC,
    LPDIRECTSOUNDCAPTUREBUFFER8 * ppDSCB8)
{
    HRESULT hr;
    DSCBUFFERDESC               dscbd;
    LPDIRECTSOUNDCAPTUREBUFFER  pDSCB;
    WAVEFORMATEX                wfx =
    { WAVE_FORMAT_PCM, 2, 44100, 176400, 4, 16, 0 };
    // wFormatTag, nChannels, nSamplesPerSec, mAvgBytesPerSec,
    // nBlockAlign, wBitsPerSample, cbSize

    if ((NULL == pDSC) || (NULL == ppDSCB8)) return E_INVALIDARG;
    dscbd.dwSize = sizeof(DSCBUFFERDESC);
    dscbd.dwFlags = 0;
    dscbd.dwBufferBytes = wfx.nAvgBytesPerSec;
    dscbd.dwReserved = 0;
    dscbd.lpwfxFormat = &wfx;
    dscbd.dwFXCount = 0;
    dscbd.lpDSCFXDesc = NULL;

    if (SUCCEEDED(hr = pDSC->CreateCaptureBuffer(&dscbd, &pDSCB, NULL)))
    {
        hr = pDSCB->QueryInterface(IID_IDirectSoundCaptureBuffer8, (LPVOID*)ppDSCB8);
        pDSCB->Release();
    }
    return hr;
}

HRESULT Lock(
    DWORD dwOffset,
    DWORD dwBytes,
    LPVOID * ppvAudioPtr1,
    LPDWORD  pdwAudioBytes1,
    LPVOID * ppvAudioPtr2,
    LPDWORD pdwAudioBytes2,
    DWORD dwFlags
)

HRESULT GetCurrentPosition(
    LPDWORD pdwCapturePosition,
    LPDWORD pdwReadPosition
)
*/
