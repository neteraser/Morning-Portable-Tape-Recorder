//=============================================================================
// main.cpp

#include "main.h"

#include "log.h"

#include "xml/tinyxml.h"
#include "AudioFile.h"
//#include "bass/bass.h"
//#include "bass/bassenc.h"
#include "audiolibwrapper.h"

#include "recorder.h"

#include "resource.h"
#include "UpdatingResources.h"
#include "resourcecompiler.h"

#include <vector>
#include <map>
#include <string>

#include <thread>
#include <chrono>

HWND mainWnd;

#define RECORDER_VERSION_STRING "Version v.0.4.0"

const char* diskName = "PortableTapeRecorder";
const char* diskDesc = "Portable Tape Recorder";

typedef unsigned int		uint;
typedef unsigned short  uint16;
typedef unsigned char		uint8;

__int64 timeTicks = 0;
int timeDt = 0;
float timeFps = 0;
const int timeMaximumFps = 30; // rounds to 60

int windowWidth = 900;
int windowHeight = 360;
int windowLeft = 0;
int windowTop = 0;

bool disableWindowMovement = false;

IDirectInput* di = 0;
IDirectInputDevice* keyboard = 0;
IDirectInputDevice* mouse = 0;

#define REC_PI 3.14159265358979323846

float DegreesToRadians(float degrees)
{
	return degrees * REC_PI / 180.0f;
}

bool InitInput(HINSTANCE instance, HWND context)
{
	DirectInput8Create(instance, DIRECTINPUT_VERSION, 
		IID_IDirectInput8A, (void**)&di, 0);

	if(!di)
    return false;

  di->CreateDevice(GUID_SysKeyboard, &keyboard, 0);
  if(!keyboard)
	return false;

	keyboard->SetDataFormat(&c_dfDIKeyboard);
	keyboard->SetCooperativeLevel(context, DISCL_EXCLUSIVE);
	keyboard->Acquire();

	di->CreateDevice(GUID_SysMouse, &mouse, 0);
	if (!mouse)
		return false;

	mouse->SetDataFormat(&c_dfDIMouse);
	mouse->SetCooperativeLevel(context, DISCL_EXCLUSIVE);
	mouse->Acquire();

	di->Release();
	di = 0;

	return true;
}

float mouseX = 0;
float mouseY = 0;
float mousePrevX = 0;
float mousePrevY = 0;

int cursorX = 0;
int cursorY = 0;

char kbbuf[256];
char kbbufPrev[256];

DIMOUSESTATE mstate = { 0 };
bool bMouseClick = false;
bool bMouseDoubleClick = false;

bool MouseL()
{
	return mstate.rgbButtons[0] & 255;
}

bool MouseClick()
{
	return bMouseClick;
}

bool MouseDoubleClick()
{
	return bMouseDoubleClick;
}

bool MouseInBox_WH(int x, int y, int w, int h)
{
	if (mouseX > x && mouseX < x + w && mouseY > y && mouseY < y + h)
		return true;

	return false;
}

bool MouseInACircle(float cx, float cy, float r)
{
	float dx = fabs(cx - mouseX);
	float dy = fabs(cy - mouseY);
	float dr = sqrtf(dx * dx + dy * dy);
	if (dr < r)
		return true;
	return false;
}

bool MouseClickInBox(int x, int y, int w, int h)
{
	if (MouseClick() && MouseInBox_WH(x, y, w, h))
		return true;
	return false;
}

int MouseMoveX()
{
	return mstate.lX;
}

int MouseMoveY()
{
	return mstate.lY;
}

bool KeyPressed(int k)
{
	if (!keyboard)
		return false;
	else
		return kbbuf[k] & 0x80;
}

//static __int64 keyPressTime[256] = { 0 };

bool KeyTrig(int k)
{
	if (!keyboard)
		return false;
	else
	{
		bool kb = (kbbuf[k] & 0x80);
		bool kbprev = (kbbufPrev[k] & 0x80);
		if ( !kb && kbprev )
		{
			kbbuf[k] = 0;
			kbbufPrev[k] = 0;
//			keyPressTime[k] = timeTicks;
			return true;
		}
	}
	return false;
}

void UpdateInput(HWND context)
{
	bMouseClick = false;
	if ( GetFocus() != context )
		return;

	POINT cursorpt;
  GetCursorPos(&cursorpt);

	cursorX = cursorpt.x;
	cursorY = cursorpt.y;
  
	ScreenToClient(context, &cursorpt);
  
	RECT clientRect;
  GetClientRect(context, &clientRect);

	mousePrevX = mouseX;
	mousePrevY = mouseY;

	mouseX = (float)cursorpt.x / (float)(clientRect.right - clientRect.left) * windowWidth;
	mouseY = (float)cursorpt.y / (float)(clientRect.bottom - clientRect.top) * windowHeight;

	if( keyboard ) 
	{
		CopyMemory(kbbufPrev, kbbuf, 256);
		HRESULT hr = keyboard->GetDeviceState(256, (void*)&kbbuf);
		if(FAILED(hr))
		{
			ZeroMemory(kbbuf, 256);
			ZeroMemory(kbbufPrev, 256);
		}
	}
	else
	{
		CopyMemory(kbbufPrev, kbbuf, 256);
		ZeroMemory(kbbuf, 256);
	}

	if(mouse) 
	{
		mouse->GetDeviceState(sizeof(DIMOUSESTATE), &mstate);
	
		static bool isUp = false;
		static __int64 clickTime = 0;
		static __int64 pressTime = 0;

		bMouseClick = false;

		if( MouseL() ) 
		{
			if(isUp)
				pressTime = timeTicks;
			isUp = false;
		}
		else
		{
			if(!isUp) 
			{
				if(timeTicks - pressTime < 250)
					bMouseClick = true;
			}
			isUp = true;
		}

		bMouseDoubleClick = false;

		if(bMouseClick && ( timeTicks - clickTime < 250))
		{
			bMouseDoubleClick = true;
		}

		if(bMouseClick)
		{
			clickTime = timeTicks;
		}
	}

}

IDirect3DDevice9* d3dd = 0;

D3DPRESENT_PARAMETERS d3dpp;

IDirect3DVertexDeclaration9* decl_vtx_t;

ID3DXFont* font;

bool InitD3D(HWND context)
{
	IDirect3D9* d3d9;
	d3d9 = Direct3DCreate9(D3D_SDK_VERSION);

	D3DDISPLAYMODE dm = { 0 };
	//if(true)
	d3d9->GetAdapterDisplayMode(0, &dm);
	//d3d9->EnumAdapterModes(0, D3DFMT_A8R8G8B8, 0, &dm);

	dm.Width = windowWidth;
	dm.Height = windowHeight;

	D3DPRESENT_PARAMETERS pp = {
		dm.Width, 
		dm.Height,
		dm.Format, 
		1,
		D3DMULTISAMPLE_NONE, 
		0,
		D3DSWAPEFFECT_DISCARD, 
		context, 
		true,
		true, 
		D3DFMT_D16, 
		D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL, 
		0,//dm.RefreshRate, 
		D3DPRESENT_INTERVAL_IMMEDIATE
	};
	d3dpp=pp;

	d3dd = 0;
	d3d9->CreateDevice(0, D3DDEVTYPE_HAL, context, 
	D3DCREATE_HARDWARE_VERTEXPROCESSING , &pp, &d3dd);

	if(!d3dd)
	{
		d3d9->CreateDevice(0, D3DDEVTYPE_HAL, context, 
			D3DCREATE_MIXED_VERTEXPROCESSING, &pp, &d3dd);

		if(!d3dd)
		{
			d3d9->CreateDevice(0, D3DDEVTYPE_HAL, context, 
				D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &d3dd);

			if(!d3dd)
			{
				d3d9->CreateDevice(0, D3DDEVTYPE_REF, context, 
					D3DCREATE_SOFTWARE_VERTEXPROCESSING, &pp, &d3dd);
			}
		}
	}

	if(!d3dd)
		return false;

  D3DVERTEXELEMENT9 elem[] = { 
		0, 0, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0,
		0, 8,  D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITIONT, 0,
		D3DDECL_END()
	};

  decl_vtx_t = 0;
  d3dd->CreateVertexDeclaration(elem, &decl_vtx_t);
  if(!decl_vtx_t)
    return false;

	d3dd->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1 | D3DFVF_TEXCOORDSIZE2(0));
  d3dd->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
  d3dd->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
  d3dd->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

	//

  d3dd->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
  d3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  d3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

  d3dd->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
  d3dd->SetRenderState( D3DRS_LIGHTING, FALSE );
  d3dd->SetRenderState( D3DRS_ZENABLE,  FALSE );

  d3dd->SetRenderState(D3DRS_ALPHAREF, (DWORD)0x00000000);
  d3dd->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE); 
  d3dd->SetRenderState(D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL);

	//

  d3dd->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
  d3dd->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	d3dd->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
  
  d3dd->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
  d3dd->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
  d3dd->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);

	//

	D3DXCreateFont(d3dd, 14, 6,  0, 1, 0, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, 5, 0, "Verdana", &font);

	return true;
}

void DrawText(ID3DXFont* font, int l, int t, int r, int b, unsigned clr, const char* str, ... ){
  va_list list;
  va_start(list, str);

  char buf[1024];
  vsprintf(buf, str, list);

  RECT rr;
  rr.left = l;
  rr.top = t;
  rr.right = r;
  rr.bottom = b;
  
  font->DrawTextA(0, buf, strlen(buf), &rr, DT_CENTER|DT_VCENTER, clr);

  va_end(list);
}

void DrawStartupScreen()
{
	d3dd->Clear(0,0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xFF000000, 0, 0);
	d3dd->BeginScene();
	
	DrawText(font, 0, 0, windowWidth, windowHeight, D3DCOLOR_RGBA(255, 255, 255, 96), "Loading... please wait");

	d3dd->EndScene();
	if(d3dd->Present(0, 0, 0, 0) == D3DERR_DEVICELOST ) {
	#ifndef _DEBUG
		PostQuitMessage(0);
	#endif
	}
}


void SetBlendNormal()
{
	d3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
  d3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
}

void SetBlendAdditive()
{
	d3dd->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	d3dd->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
}

void SetColor(uint r, uint g, uint b, uint a)
{
	d3dd->SetRenderState(D3DRS_TEXTUREFACTOR, D3DCOLOR_RGBA((uint)(r), (uint)(g), (uint)(b), (uint)(a)) );
}

void SetAlpha(uint alpha)
{
	SetColor(255, 255, 255, alpha);
}

void ResetColor()
{
	SetColor(255, 255, 255, 255);
}

Picture pictures[256];
int picturesNumber = 1;

int LoadPicture(const char* image)
{
	IDirect3DTexture9* tx = 0;

	D3DXCreateTextureFromFileEx(d3dd, image, D3DX_FROM_FILE, D3DX_FROM_FILE, 1, 0, 
		D3DFMT_A8R8G8B8, D3DPOOL_MANAGED, D3DX_FILTER_NONE,
		D3DX_FILTER_NONE , 0x00000000, 0, 0, &tx);
	
	if(!tx)
	{
		pictures[0].texture = 0;
		pictures[0].width = 0;
		pictures[0].height = 0;

		WriteToLog("Failed to load picture: %s", image);

		return 0;
	}

	D3DSURFACE_DESC desc;
	tx->GetLevelDesc(0, &desc);

	int r = picturesNumber;

	pictures[r].texture = tx;

	pictures[r].width = (float)desc.Width;
	pictures[r].height = (float)desc.Height;

	picturesNumber += 1;

	return r;
}

int LoadPicture(const std::string& imageStr)
{
	return LoadPicture(imageStr.c_str());
}

void SetPicture(int n)
{
	if (n < 0)
		return;
	d3dd->SetTexture(0, pictures[n].texture);
}

typedef struct {
  float x, y, z, w;
  float t0, t1;
} vtx_t;

unsigned short idx[6] = {
  0, 1, 2, 1, 2, 3
};

void DrawPicture(int n, float i, float j, float ii = 0, float ij = 0, int ofx=0, int ofy=0, bool draw = true /* ignored */, float tx = 1.0f, float ty = 1.0f, float t0x = 0.0f, float t0y = 0.0f) 
{
	if(ii <= 0)
	{
		ii = pictures[n].width;
	}

	if(ij <= 0)
	{
		ij = pictures[n].height;
	}

  vtx_t v[4];

  v[0].t0 = t0x;//+0.5f/pics[n].wx;
  v[0].t1 = t0y;//+0.5f/pics[n].wy;
  v[0].x  = (float)i-0.5f;
  v[0].y  = (float)j-0.5f;
  v[0].z  = 0;
  v[0].w  = 1;


  v[1].t0 = tx; // +0.5f/pics[n].wx;//ofx+ii;
  v[1].t1 = t0y;// +0.5f/pics[n].wy;//ofy;
  v[1].x  = (float)i+(float)ii-0.5f;
  v[1].y  = (float)j-0.5f;
  v[1].z  = 0;
  v[1].w  = 1;


  v[2].t0 = t0x;//+0.5f/pics[n].wx;//ofx;
  v[2].t1 = ty;//+0.5f/pics[n].wy;//ofy+ij;
  v[2].x  = (float)i-0.5f;
  v[2].y  = (float)j+(float)ij-0.5f;
  v[2].z  = 0;
  v[2].w  = 1;

  v[3].t0 = tx;//+0.5f/pics[n].wx;//ofx+ii;
  v[3].t1 = ty;//+0.5f/pics[n].wy;//ofy+ij;
  v[3].x  = (float)i+(float)ii-0.5f;
  v[3].y  = (float)j+(float)ij-0.5f;
  v[3].z  = 0;  
  v[3].w  = 1;

  SetPicture(n);
  
  d3dd->DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 4, 2, idx, D3DFMT_INDEX16, &v[0], sizeof(vtx_t));
}

bool noButtonIsOver = true;

int DrawButton(int butPictures[2], int x, int y, bool clickOrDown = true, bool forceOver = false)
{
	int isOver = 0;
	int result = 0;

	if( MouseInBox_WH(x, y, pictures[butPictures[0]].width, pictures[butPictures[0]].height) )
	{
		isOver = 1;

		if ( clickOrDown ? MouseClick() : MouseL() )
		{
			result = 1;
		}

		noButtonIsOver = false;
	}

	if (forceOver)
		isOver = 1;

	DrawPicture( butPictures[isOver], x, y);

	return result;
}

bool DrawChecker(int* checkerpics, int x, int y, bool state)
{
	bool result = false;

	int pic = state ? 1 : 0;

	if (MouseInBox_WH(x, y, pictures[checkerpics[pic]].width, pictures[checkerpics[pic]].height))
	{
		if (MouseClick())
			result = true;
	}

	DrawPicture(checkerpics[pic], x, y);

	return result;
}

/////////////////////////////

Sound sounds[16];
int soundsNumber = 1;
float* soundallocations[16];
int soundallocationsNumber = 0;

int LoadSound(const char* filename)
{
	AudioFile<float> af;
	if( ! af.load(filename) )
	{
		WriteToLog("Audio File failed at loading a sound: %s", filename);

		Sound sndret;
		if (RetrieveSoundFromCode(filename, sndret))
		{
			WriteToLog("Retrieved %s sound from code.", filename);
			sounds[soundsNumber] = sndret;
			return soundsNumber++;
		}

		sounds[0].snd = 0;
		sounds[0].length = 0;
		sounds[0].sampledata = 0;

		return 0;
	}
	int lenb = af.getNumSamplesPerChannel();//BASS_ChannelGetLength(mybass, BASS_POS_BYTE);
	if (lenb <= 0)
	{

		sounds[0].snd = 0;
		sounds[0].length = 0;
		sounds[0].sampledata = 0;

		return 0;
	}
	sounds[soundsNumber].snd = 0;
	sounds[soundsNumber].length = lenb;// / sizeof(float);
	float* newalloc = (float*)malloc(sounds[soundsNumber].length * sizeof(float));
	soundallocations[soundallocationsNumber++] = newalloc;
	sounds[soundsNumber].sampledata = newalloc;//new float[sounds[soundsNumber].length];
	//BASS_ChannelGetData(mybass, sounds[soundsNumber].sampledata, lenb | BASS_DATA_FLOAT);
	//BASS_SampleGetData(mybass, sounds[soundsNumber].sampledata);
	for (int i = 0; i < lenb; ++i)
		sounds[soundsNumber].sampledata[i] = af.samples[0][i];

	BakeSoundToCode(sounds[soundsNumber], filename);

	return soundsNumber++;
}

int LoadSound(std::string filename)
{
	return LoadSound(filename.c_str());
}

void FreeSounds()
{
	for (int i = 0; i < soundallocationsNumber; ++i)
	{
		free(soundallocations[i]);
		//delete [] sounds[i].sampledata;
	}
	soundallocationsNumber = 0;
}

//////////////////////////

int beepforme;


////////////////////////////

HMENU menu;
NOTIFYICONDATA notifyIconData;
UINT WM_TASKBARCREATED = 0 ;

#define ID_TRAY_APP_ICON                5000
#define ID_TRAY_EXIT_CONTEXT_MENU_ITEM  3000
#define WM_TRAYICON ( WM_USER + 1 )

void InitNotifyIconData(HINSTANCE hInst)
{
  memset( &notifyIconData, 0, sizeof( NOTIFYICONDATA ) ) ;

  notifyIconData.cbSize = sizeof(NOTIFYICONDATA);

  /////
  // Tie the NOTIFYICONDATA struct to our
  // global HWND (that will have been initialized
  // before calling this function)
  notifyIconData.hWnd = mainWnd;
  // Now GIVE the NOTIFYICON.. the thing that
  // will sit in the system tray, an ID.
  notifyIconData.uID = ID_TRAY_APP_ICON;
  // The COMBINATION of HWND and uID form
  // a UNIQUE identifier for EACH ITEM in the
  // system tray.  Windows knows which application
  // each icon in the system tray belongs to
  // by the HWND parameter.
  /////

  /////
  // Set up flags.
  notifyIconData.uFlags = NIF_ICON | // promise that the hIcon member WILL BE A VALID ICON!!
    NIF_MESSAGE | // when someone clicks on the system tray icon,
    // we want a WM_ type message to be sent to our WNDPROC
    NIF_TIP;      // we're gonna provide a tooltip as well, son.

  notifyIconData.uCallbackMessage = WM_TRAYICON; //this message must be handled in hwnd's window procedure. more info below.

  // Load da icon.  Be sure to include an icon "green_man.ico" .. get one
  // from the internet if you don't have an icon
  notifyIconData.hIcon = (HICON)LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)) ;

  // set the tooltip text.  must be LESS THAN 64 chars
	strcpy(notifyIconData.szTip, TEXT(diskDesc));

	WM_TASKBARCREATED = RegisterWindowMessageA("TaskbarCreated") ;
}

void Minimize()
{
  // add the icon to the system tray
  Shell_NotifyIcon(NIM_ADD, &notifyIconData);

  // ..and hide the main window
  ShowWindow(mainWnd, SW_HIDE);
}

void Restore()
{
  // Remove the icon from the system tray
  Shell_NotifyIcon(NIM_DELETE, &notifyIconData);

  // ..and show the window
  ShowWindow(mainWnd, SW_SHOW);
}

LRESULT WINAPI ProccessWindow(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch(Msg)
  {
  case WM_CREATE:

    // create the menu once.
    // oddly, you don't seem to have to explicitly attach
    // the menu to the HWND at all.  This seems so ODD.
    menu = CreatePopupMenu();

    AppendMenu(menu, MF_STRING, ID_TRAY_EXIT_CONTEXT_MENU_ITEM,  TEXT( "Exit" ) );

    break;

  //case WM_SYSCOMMAND:
  //  switch( wParam & 0xfff0 )  // (filter out reserved lower 4 bits:  see msdn remarks http://msdn.microsoft.com/en-us/library/ms646360(VS.85).aspx)
  //  {
  //  case SC_MINIMIZE:
  //  case SC_CLOSE:  // redundant to WM_CLOSE, it appears
  //    Minimize() ;
  //    return 0 ;
  //    break;
  //  }
  //  break;

  // Our user defined WM_TRAYICON message.
  // We made this message up, and we told
  //
  case WM_TRAYICON:
    {
      //printf( "Tray icon notification, from %d\n", wParam ) ;

      switch(wParam)
      {
      case ID_TRAY_APP_ICON:
        //printf( "Its the ID_TRAY_APP_ICON.. one app can have several tray icons, ya know..\n" ) ;
        break;
      }

      // the mouse button has been released.

      // I'd LIKE TO do this on WM_LBUTTONDOWN, it makes
      // for a more responsive-feeling app but actually
      // the guy who made the original post is right.
      // Most apps DO respond to WM_LBUTTONUP, so if you
      // restore your window on WM_LBUTTONDOWN, then some
      // other icon will scroll in under your mouse so when
      // the user releases the mouse, THAT OTHER ICON will
      // get the WM_LBUTTONUP command and that's quite annoying.
      if (lParam == WM_LBUTTONUP)
      {
        //printf( "You have restored me!\n" ) ;
        Restore();
      }
      else if (lParam == WM_RBUTTONDOWN) // I'm using WM_RBUTTONDOWN here because
      {
        //printf( "Mmm.  Let's get contextual.  I'm showing you my context menu.\n" ) ;

        // it gives the app a more responsive feel.  Some apps
        // DO use this trick as well.  Right clicks won't make
        // the icon disappear, so you don't get any annoying behavior
        // with this (try it out!)

        // Get current mouse position.
        POINT curPoint ;
        GetCursorPos( &curPoint ) ;

        // should SetForegroundWindow according
        // to original poster so the popup shows on top
        SetForegroundWindow(hWnd); 

        // TrackPopupMenu blocks the app until TrackPopupMenu returns
        //printf("calling track\n");

        UINT clicked = TrackPopupMenu(
          menu,
          TPM_RETURNCMD | TPM_NONOTIFY, // don't send me WM_COMMAND messages about this window, instead return the identifier of the clicked menu item
          curPoint.x,
          curPoint.y,
          0,
          hWnd,
          NULL
        );
        
				//printf("returned from call to track\n");

        // Original poster's line of code.  Haven't deleted it,
        // but haven't seen a need for it.
        //SendMessage(hwnd, WM_NULL, 0, 0); // send benign message to window to make sure the menu goes away.
        if (clicked == ID_TRAY_EXIT_CONTEXT_MENU_ITEM)
        {
          // quit the application.
          //printf("I have posted the quit message, biatch\n");
          PostQuitMessage( 0 ) ;
        }
      }
    }
		break;

  case WM_DESTROY:
    PostQuitMessage(0);
    break;
  }

  return DefWindowProc(hWnd, Msg, wParam, lParam);
}

std::string devName = "";
std::string devRecName = "";

int curDevNum = -1;
int curRecDevNum = -1;

std::vector<std::string> allDevNames;
std::vector<std::string> allDevRecNames;

void RefreshDeviceChoice()
{
	curDevNum = -1;
	curRecDevNum = -1;
	devName = "";
	devRecName = "";
	allDevNames.clear();
	allDevRecNames.clear();
}

void DeviceCheck()
{
	gaudio->EnumerateDevices();

	//RecorderDeviceRetrieve(-1, -1);

	//allDevNames.clear();
	//allDevRecNames.clear();

	allDevNames.resize(0);
	allDevRecNames.resize(0);

	//allDevNames.push_back("No sound");

	curDevNum = gaudio->GetDevice();
	if (curDevNum == -1)
	{
		WriteToLog("Audio Library GetDevice at DeviceCheck failed");
	}
	curRecDevNum = gaudio->GetRecDevice();
	if (curRecDevNum == -1)
	{
		WriteToLog("AudioLibrary GetRecDevice at DeviceCheck failed");
	}

	allDevNames = gaudio->devices;
	allDevRecNames = gaudio->recDevices;
}

///////////////////////////////////////////////////////////////////////////////

int tapeSlowdownSamples = 2;
int tapeSlowdownMode2Samples = 10;
int adcIntMaxNatural = 32500;

	//////////////////////////////////////////////////////////////////

int
normalizeX = 215, normalizeY = 450,
exportX = 215, exportY = 490,
addriaaX = 375, addriaaY = 465,
undoFXX = 375, undoFXY = 495,
recX = 23, recY = 267,
playX = 90, playY = 470,
zoomX = 330, zoomY = 25,
hideX = 456, hideY = 267,
quitX = 473, quitY = 267,
equalizerY = 255, eqCropLeft = 1, eqCropRight = 0,
meterX = 850, meterY = 110, meterWidth = 36, meterHeight = 320, meterOffsetX = 3, meterOffsetY = 5,
trackX = 25, trackY = 125, trackWidth = 750, trackHeight = 100,
remasterX = 460, remasterY = 460,
applyX = 680, applyY = 18,
settingsX = 350, settingsY = 25,
loopX = 180, loopY = 480,
onoffX = 400, onoffY = 10,
regulatorX = 600, regulatorY = 465, regulatorDiameter = 60,
waveformcolorX = 25, waveformcolorY = 110,
bassX = 30, bassY = 480, portyX = 210, portyY = 480,
delayslowX = 480, delayslowY = 480, delayfastX = 510, delayfastY = 480;

double regulatorDefaultValue = 0.5;

std::string
quitImage, quitOverImage,
hideImage, hideOverImage,
normalizeImage, normalizeOverImage,
limitImage, limitOverImage,
recImage, recOverImage,
stopImage, stopOverImage,
playImage, playOverImage,
stopplayImage, stopplayOverImage,
zoomInImage, zoomInOverImage,
zoomOutImage, zoomOutOverImage,
zoomFullImage, zoomFullOverImage,
exportImage, exportOverImage,
exportMp3Image, exportMp3OverImage,
addriaaImage, addriaaOverImage,
undofxImage, undofxOverImage,
remasterImage, remasterOverImage,
remasterplayingImage, remasterplayingOverImage,
meterImage,
settingsImage, settingsOverImage,
applyImage, applyOverImage,
loopOnImage, loopOnOverImage,
loopOffImage, loopOffOverImage,
onImage, onOverImage,
offImage, offOverImage,
regulatorImage, regulatorPtrImage, regulatorPressImage,
bassLibImage, bassLibImageOn,
portyLibImage, portyLibImageOn,
delaySlowImage, delaySlowImageOn,
delayFastImage, delayFastImageOn;


std::string exportartist = "Tape Recorder";

std::string beepSound = "beepforme.wav";

int back;
int settingsback;
int quit[2];
int hide[2];
int normalize[2];
int limit[2];
int exportbut[2];
int exportmp3but[2];
int addriaa[2];
int undofx[2];
int rec[2];
int play[2];
int stopplay[2];
int zoomIn[2];
int zoomOut[2];
int zoomFull[2];
int remaster[2];
int remasterPlaying[2];
int meter;
int stop[2];
int settingsimg[2];
int applyimg[2];
int loopon[2];
int loopoff[2];
int onimg[2];
int offimg[2];
int regulatorimg, regulatorptrimg, regulatorpressimg;
int arrowup;
int arrowdown;
int basslib[2];
int portylib[2];
int delayslow[2];
int delayfast[2];
int px1;


//////////////
// 
// 

int spFrame = 1;
int normalize_mode = 1;

////////////////////////////////////////////////////////////////////////////////
// 
// 
int spectrumLength = 0;

float spectrum[4096];

float pointsY[4096];
float prevPointsY[4096];

float pointsX[4096];
int	pointsN = 0;

////////////////////////////////////////////////////////////////////////////////

void DrawMainScreen()
{
	if (DrawButton(remastermode ? remaster : remasterPlaying, remasterX, remasterY))
	{
		if (remastermode != 0)
		{
			StartRemaster();
			remastermode = 0;
		}
		else
		{
			StopRemaster();
			remastermode = 1;
		}
	}


	///////////////////////////////////////////////////////////

	static int zoomMode = 0;
	int* zoomPic[3] = { zoomIn, zoomOut, zoomFull };

	if (DrawButton(zoomPic[zoomMode], zoomX, zoomY))
	{
		/*if (zoomInOrOut)
		{

		}
		zoomInOrOut = !zoomInOrOut;*/
		zoomMode += 1;
		if (zoomMode >= 3)
			zoomMode = 0;
	}

	bool canclick = true;

	if (IsRecorderPlaying())
	{
		canclick = false;
		SetAlpha(128);
	}

	if (DrawButton(recOrStop ? rec : stop, recX, recY) && canclick)
	{
		if (recOrStop)
		{
			//BASS_ChannelPlay(songs[n].song, FALSE);
			StartRec(baserec);
			recOrStop = 0;
		}
		else
		{
			StopRec(baserec);
			recOrStop = 1;
		}
	}

	ResetColor();

	canclick = true;

	if (IsRecorderWriting())
	{
		SetAlpha(128);
		canclick = false;
	}

	if (DrawButton(playOrStop ? play : stopplay, playX, playY) && canclick)
	{
		if (playOrStop)
		{
			//BASS_ChannelPlay(songs[n].song, FALSE);
			PlayRec();
			playOrStop = 0;
		}
		else
		{
			StopPlayingRec();
			playOrStop = 1;
		}
	}

	ResetColor();

	canclick = true;

	if (!RecorderCanDoFX())
	{
		SetAlpha(128);
		canclick = false;
	}

	if (DrawButton(normalize_mode ? normalize : limit, normalizeX, normalizeY) && canclick)
	{
		if (normalize_mode == 1)
			NormalizeRec(baserec);
		else
			LimitRec(baserec);
		normalize_mode = !normalize_mode;
	}

	if (DrawButton(addriaa, addriaaX, addriaaY) && canclick)
	{
		AddRIAAToRec(baserec);
	}

	ResetColor();

	canclick = true;

	if (!RecorderCanUndo())
	{
		SetAlpha(128);
		canclick = false;
	}

	if (DrawButton(undofx, undoFXX, undoFXY) && canclick)
	{
		UndoFXFromRec();
		normalize_mode = 1;
	}

	ResetColor();

	canclick = true;

	if (!IsRecorderIdling())
	{
		canclick = false;
		SetAlpha(128);
	}

	static bool exportwavormp3 = true;

	if (DrawButton(exportwavormp3 ? exportbut : exportmp3but, exportX, exportY) && canclick)
	{
		SaveRec(exportwavormp3);
		exportwavormp3 = !exportwavormp3;
	}

	ResetColor();

	if (DrawButton(settingsimg, settingsX, settingsY))
	{
		if (IsRecorderWriting())
			StopRec(baserec);
		if (IsRecorderPlaying())
			StopPlayingRec();
		if (IsRecorderRemastering())
			StopRemaster();

		spFrame = 2;
	}

	if (DrawButton(quit, quitX, quitY))
	{
		PostQuitMessage(0);
	}

	if (DrawButton(hide, hideX, hideY))
	{
		Minimize();
		//ShowWindow(mainWnd, SW_MINIMIZE );
	}

	if (noButtonIsOver)
	{
		SetCursor(LoadCursor(0, IDC_ARROW));
	}
	else
	{
		SetCursor(LoadCursor(0, IDC_HAND));
	}

	/*
	if (KeyTrig(DIK_D))
	{
		RecorderDeviceRetrieve();
	}*/

	if (IsRecorderIdling())
	{
		if (KeyTrig(DIK_R))
		{
			StartRec(baserec);
		}
		else
		if (KeyTrig(DIK_SPACE))
		{
			PlayRec();
		}
	}
	else
	{
		if (KeyTrig(DIK_R) && IsRecorderWriting())
		{
			StopRec(baserec);
		}
		
		if (KeyTrig(DIK_SPACE) )
		{
			if (IsRecorderPlaying())
				StopPlayingRec();
			else
				if (IsRecorderWriting())
					StopRec(baserec);
		}
	}

	DWORD result = 0;//BASS_ChannelGetData( songs[n].song, spectrum, bassSpectrumLength);

	for (int i = 0; i < spectrumLength; ++i)
	{
		int rsppos = 0;
		rsppos = baserec.unipos + i - spectrumLength;
		if (rsppos < 0)
			rsppos = 0;

		spectrum[i] = baserec.recbuf[rsppos];
	}

	//BASS_Update(1);
	/*
	if (BASS_GetDevice() != bassdevice )
	{
		MessageBox(mainWnd, "Playing device lost!", "Error", MB_OK);
		//RecorderDeviceRetrieve();
	}

	if (BASS_RecordGetDevice() != bassrecorddevice )
	{
		MessageBox(mainWnd, "Recording device lost!", "Error", MB_OK);
		//RecorderDeviceRetrieve();
	}*/

	//BassDeviceCheck();

	/*
	if (KeyTrig(DIK_D))
	{
		RecorderDeviceRetrieve();
	}
	*/

	//BassDeviceCheck();

	float spmax = 0;

	for (int i = 1; i < spectrumLength; ++i)
	{
		if (spectrum[i] > spmax)
		{
			spmax = spectrum[i];
		}
	}

	for (int i = 0; i < spectrumLength; ++i)
	{
		spectrum[i] = spectrum[i] / spmax * 48.0f;
	}

	pointsN = 0;
	int lastX = -100;

	float coeff = (float)windowWidth / logf(windowWidth);

	for (int i = 1; i < windowWidth; ++i)
	{
		float x = //logf( (float) i / 1024.0f * 44100.0f ) * 79.48f - 298.94f;
			logf((float)i) * coeff;

		if (x - lastX > 1)
		{
			//SetColor(255, 0, 0, 255);
			//DrawPicture( px1, x, equalizerY - spectrum[ i ] - 3 , 2, 2);

			pointsY[pointsN] = spectrum[i];

			if (pointsY[pointsN] < 0)
				pointsY[pointsN] = 0;

			pointsX[pointsN] = x;
			pointsN += 1;

			if (pointsN > 255)
				pointsN = 255;

			lastX = x;
		}
	}

	ResetColor();

	for (int i = 0; i < 256; ++i)
	{
		float prev = prevPointsY[i];
		prevPointsY[i] = pointsY[i];
		pointsY[i] = (pointsY[i] + prev) / 2.0f;
	}

	int pointI = 0;
	float prevY = 0;

	for (float i = eqCropLeft; i < (float)(windowWidth - eqCropRight); i += 0.33f)
	{
		int i0, i1, i2, i3;
		i0 = pointI - 2;
		i1 = pointI - 1;
		i2 = pointI;
		i3 = pointI + 1;

		if (i0 < 0)
			i0 = 0;

		if (i1 < 0)
			i1 = 0;

		if (i2 > windowWidth - 1)
			i2 = windowWidth - 1;

		if (i3 > windowWidth - 1)
			i3 = windowWidth - 1;

		float d = pointsX[i2] - pointsX[i1];

		float t, y;

		if (d > 0)
		{
			t = (i - pointsX[i1]) / d;
			y = CatmullRom1D(t, pointsY[i0], pointsY[i1], pointsY[i2], pointsY[i3]);
		}
		else
		{
			t = 0.0f;
			y = pointsY[i1];
		}

		if (i > pointsX[pointI])
		{
			pointI += 1;
		}

		if (y < 0)
			y = 0;

		ResetColor();
		//SetAlpha(( 24.0f * transitionAlpha ) / 255.0f );
		//SetBlendAdditive();

		DrawPicture(px1, i, equalizerY - (prevY + y) / 2.0f, 1.0f, 1.0f);

		prevY = y;
	}

		// equalizer

//			if (spFrame == 1)
//			{
//			}

			//////////////////////////////////////////////////
			//trackWidth = 512;

		int waveformlen = 250;
		if (zoomMode == 1)
		{
			waveformlen = baserec.bufpos / 48000 + 1;
			if (IsRecorderWriting())
				waveformlen += 2;
		}

		int samplesperpx = 48000 * waveformlen / trackWidth;

		if (zoomMode == 2)
			samplesperpx = 1;

		int curpos = 0;
		float curpos_rt = 0;
		if (IsRecorderPlaying())
		{
			curpos_rt = timeGetTime() - streamrec.lastplaycallback_time;
			curpos_rt /= 1000.0f;
			curpos_rt *= 48000.0;
		}
		curpos = (baserec.unipos - 4800 + curpos_rt) / samplesperpx;
		if (zoomMode == 1 && (IsRecorderIdling()))
		{
			curpos = ((float)baserec.peakpos / (float)(waveformlen * 48000)) * ((float)trackWidth);// / samplesperpx;
		}
		if (curpos < 0)
			curpos = 0;

		int startpos = 0;

		if (zoomMode == 2)
		{
			startpos = curpos;
			if (IsRecorderWriting())
			{
				startpos -= trackWidth;
				if (startpos < 0)
					startpos = 0;
			}
			else
			if (IsRecorderIdling())
			{
				startpos = baserec.peakpos - (trackWidth / 2);
				if (startpos < 0)
					startpos = 0;
				curpos = trackWidth / 2;
			}
			else
			{
				curpos = 0;
			}
		}

		static int clrwav[3][3] = { {0, 255, 0 }, {255, 255, 0}, {0, 255, 255} };
		static int currentclr = 0;

		//				SetColor(0, 0, 0, 255);
		for (int i = 0; i < 3; ++i)
		{
			int clrwavalpha = 255;

			if (MouseInBox_WH(waveformcolorX + i * 25, waveformcolorY - 2, 14, 14))
			{
				clrwavalpha = 128;
				if (MouseClick())
				{
					currentclr = i;
					RecorderSetColoration(currentclr);
				}
			}

			if (i == currentclr)
			{
				SetColor(255, 255, 255, 255);
				DrawPicture(px1, waveformcolorX + i * 25 - 1, waveformcolorY - 2, 16, 16);
			}
			else
			{
				SetColor(128, 128, 128, 255);
				DrawPicture(px1, waveformcolorX + i * 25, waveformcolorY - 1, 14, 14);
			}

			SetColor(clrwav[i][0], clrwav[i][1], clrwav[i][2], clrwavalpha);
			DrawPicture(px1, waveformcolorX + i * 25 + 1, waveformcolorY, 12, 12);
		}

		SetColor(clrwav[currentclr][0], clrwav[currentclr][1], clrwav[currentclr][2], 255);

		DrawPicture(px1, trackX - 1, trackY, trackWidth + 2, 1);
		DrawPicture(px1, trackX - 11, trackY - 5, 10, 10);
		DrawPicture(px1, trackX + trackWidth + 2, trackY - 5, 10, 10);

		for (int i = 0; i < trackWidth - 1; ++i)
		{
			int step = 1;
			if (samplesperpx >= 64)
				step = 64;
			float vminus = 0, vplus = 0;
			ComputeWaveform(baserec, (i + startpos) * samplesperpx, (i + 1 + startpos) * samplesperpx, step, vplus, vminus);
			//float sampledb = VolumeToDb(samplevalue);
			//float sampleoff = 50 - sampledb;

			//float sampledb = 100 + VolumeToDb (vplus);
			float sampledb = vplus * trackHeight;

			DrawPicture(px1, trackX + i, trackY - sampledb, 1, sampledb);

			sampledb = fabs(vminus) * trackHeight;
			//sampledb = 100 + VolumeToDb(abs(vminus));

			DrawPicture(px1, trackX + i, trackY, 1, sampledb);
		}
		//
		SetColor(0, 0, 0, 255);
		DrawPicture(px1, trackX + curpos, trackY - trackHeight / 2, 1, trackHeight);

		DrawText(font, 550, 100, 750, 125, D3DCOLOR_RGBA(255, 255, 255, 128), "Rec length: %0.3f sec", baserec.bufpos / 48000.0f);

		if (IsRecorderIdling())
		{
			float recpeakoff = 0;
			recpeakoff = baserec.peak * trackHeight;

			DrawPicture(px1, trackX + curpos, trackY - recpeakoff, 6, 1);

			DrawText(font, trackX + curpos - 50, trackY - recpeakoff - 20, trackX + curpos + 140, trackY - recpeakoff + 5, D3DCOLOR_RGBA(255, 255, 255, 128), "Peak: %0.2f", baserec.peak);
		}

		//////////////////////////////////////

		DrawText(font, 640, 50, 740, 75, D3DCOLOR_RGBA(255, 255, 255, 128), RECORDER_VERSION_STRING);

		////////////////////////////////////
		float rms = ComputeRMS(baserec, baserec.unipos - 256, baserec.unipos);
		float rmsTotal = ComputeRMS(baserec, 0, baserec.unipos); // TODO: optimize
		float db = VolumeToDb(rms);
		if (db < -99.0f)
			db = -99.0f;
		float dbTotal = VolumeToDb(rmsTotal);
		if (dbTotal < -99.0f)
			dbTotal = -99.0f;
		float dbRecHealth = -16;
		float dbscalecoeff = ((meterHeight - meterOffsetY * 2) / 100.0f);
		float dbScaleLen = (100.0 + db) * dbscalecoeff;
		float dbDelimLen = (100.0 + dbRecHealth) * dbscalecoeff;
		float dbPosX, dbPosY;

		ResetColor();
		DrawPicture(meter, meterX, meterY);

		static float metercolorcoeff_prev = 0.0f;
		float metercolorcoeff = abs(db) / 100.0f;
		metercolorcoeff = (metercolorcoeff + metercolorcoeff_prev) / 2.0f;
		metercolorcoeff_prev = metercolorcoeff;

		SetColor(255 * metercolorcoeff, 255 * (1.0f - metercolorcoeff), 0, 255);
		DrawPicture(px1, meterX + meterOffsetX, meterY + meterHeight - dbScaleLen - meterOffsetY, meterWidth - meterOffsetX * 2, dbScaleLen);

		SetColor(255, 255, 255, 255);
		DrawPicture(px1, meterX + meterOffsetX, meterY + meterHeight - dbDelimLen - meterOffsetY, meterWidth - meterOffsetX * 2, 3);

		DrawText(font, 720, 460, 720 + 150, 460 + 25, D3DCOLOR_RGBA(255, 255, 255, 128), "RMS Live: %.2f", db);
		DrawText(font, 720, 460 + 25, 720 + 150, 460 + 50, D3DCOLOR_RGBA(255, 255, 255, 128), "RMS Total: %.2f", dbTotal);

		if(KeyTrig(DIK_P))
		{
			spFrame = 2;
		}

		//
		ResetColor();
	
		static bool bloop = false;
		if (DrawButton(bloop ? loopoff : loopon, loopX, loopY))
		{
			bloop = !bloop;
			RecorderSetLoop(bloop);
		}

		canclick = !IsRecorderWriting();

		if (!canclick)
			SetAlpha(128);

		static bool rewriteOnOrOff = true;
		if (DrawButton(rewriteOnOrOff ? onimg : offimg, onoffX, onoffY) && canclick)
		{
			rewriteOnOrOff = !rewriteOnOrOff;
			RecorderSetRewriteMode(rewriteOnOrOff);
		}

		ResetColor();

		////////////////////////
		/// regulator
		static bool regulating = false;

		DrawPicture(regulating? regulatorpressimg : regulatorimg, regulatorX, regulatorY);

		//static float 
		static float ptrv = (1.0f - regulatorDefaultValue) * 300.0f + 30.0f;
		float ptrx = (cosf(DegreesToRadians(-90.0f + ptrv)) + 1.0f) / 2.0f;
		float ptry = (sinf(DegreesToRadians(-90.0f + ptrv)) + 1.0f) / 2.0f;
		float regwidth = (float) pictures[regulatorimg].width;
		float regheight = (float)pictures[regulatorimg].height;

		if (MouseInACircle(regulatorX + regwidth / 2, regulatorY + regheight / 2, regulatorDiameter))
		{
			if (MouseL())
			{
				regulating = true;
			}
			if (MouseDoubleClick())
				ptrv = (1.0f - regulatorDefaultValue) * 300.0f + 30.0f;
		}
		if (!MouseL() && regulating)
		{
			regulating = false;
		}
		if (regulating)
		{
			ptrv += (float)(MouseMoveX() + MouseMoveY()) / 2.0f;
			if (ptrv > 330)
				ptrv = 330;
			if (ptrv < 30)
				ptrv = 30;
			disableWindowMovement = true;
		}
		float regradius = regulatorDiameter / 2.0f * 0.66f;

		ptrx = ptrx * regradius * 2.0f;
		ptry = (1.0f - ptry) * regradius * 2.0f;

		// pictures[regulatorptrimg].width / 2
		ptrx = ptrx - pictures[regulatorptrimg].width / 2.0f;
		ptry = ptry - pictures[regulatorptrimg].height / 2.0f;

		ptrx += (regwidth / 2.0f - regradius);
		ptry += (regheight / 2.0f - regradius);


		DrawPicture(regulatorptrimg, (float)regulatorX + ptrx, (float)regulatorY + ptry);

		RecorderSetQFX(1.0f - (ptrv - 30.0f) / 300.0f);

		DrawText(font, 720, 460 + 50, 720 + 150, 460 + 75, D3DCOLOR_RGBA(255, 255, 255, 128), "QFX: %.2f", RecorderGetQFX());

}

////////////////////////////////////////////////////////////////////////////////

static int devPos = 0;
static int recDevPos = 0;
static const int maxDevicesOnTheScreen = 8;

void DrawSettingsScreen()
{
	//RecorderDeviceRetrieve();

	DeviceCheck();

	DrawPicture(settingsback, 0, 0);

	bool devicechanged = false;
	int devNum = curDevNum, recDevNum = curRecDevNum;

	int sizediff = allDevRecNames.size() - maxDevicesOnTheScreen;

	if (sizediff > 0)
	{
		bool isover = false;
		if (MouseInBox_WH(50, 125 + 1, 350, 18))
		{
			isover = true;
			if (MouseClick())
			{
				recDevPos -= 1;
				if (recDevPos < 0)
					recDevPos = 0;
			}
		}
		DrawPicture(px1, 50, 125 + 1, 350, 18);

		if (isover)
			SetAlpha(192);

		DrawPicture(arrowup, 200 + 15, 127);

		ResetColor();

		isover = false;
		if (MouseInBox_WH(50, 145 + maxDevicesOnTheScreen * 20 + 1, 350, 18))
		{
			isover = true;
			if (MouseClick())
			{
				recDevPos += 1;
				if (recDevPos > sizediff)
					recDevPos = sizediff;
			}
		}
		DrawPicture(px1, 50, 145 + maxDevicesOnTheScreen * 20 + 1, 350, 18);

		if (isover)
			SetAlpha(192);

		DrawPicture(arrowdown, 200 + 15, 147 + maxDevicesOnTheScreen * 20);

		ResetColor();
	}

	sizediff = allDevNames.size() - maxDevicesOnTheScreen;

	if(sizediff > 0)
	{
		bool isover = false;
		if (MouseInBox_WH(500, 125 + 1, 350, 18))
		{
			isover = true;
			if (MouseClick())
			{
				devPos -= 1;
				if (devPos < 0)
					devPos = 0;
			}
		}
		DrawPicture(px1, 500, 125 + 1, 350, 18);

		if (isover)
			SetAlpha(192);

		DrawPicture(arrowup, 650 + 15, 127);

		ResetColor();

		isover = false;
		if (MouseInBox_WH(500, 145 + maxDevicesOnTheScreen * 20 + 1, 350, 18))
		{
			isover = true;
			if (MouseClick())
			{
				devPos += 1;
				if (devPos > sizediff)
					devPos = sizediff;
			}
		}
		DrawPicture(px1, 500, 145 + maxDevicesOnTheScreen * 20 + 1, 350, 18);

		if (isover)
			SetAlpha(192);

		DrawPicture(arrowdown, 650 + 15, 147 + maxDevicesOnTheScreen * 20);

		ResetColor();

	}

	for (int i = 0; i < maxDevicesOnTheScreen; ++i)
	{
		int id = i + recDevPos;

		if (id < 0)
			break;

		if (id >= allDevRecNames.size())
			break;

		std::string devname = allDevRecNames[id];
		devname.resize(32);

		ResetColor();

		if (MouseInBox_WH(50, 145 + i * 20 + 1, 350, 18))
		{
			if (MouseClick())
			{
				if (allDevRecNames[id] != devRecName)
				{
					//BASS_RecordSetDevice(i);
					devRecName = allDevRecNames[id];
					recDevNum = id;
					devicechanged = true;
				}
			}
			else
			{
				SetColor(128, 128, 128, 255);
			}
		}

		if ( curRecDevNum == id)
			SetColor(128, 128, 128, 255);

		DrawPicture(px1, 50, 145 + i * 20 + 1, 350, 18);

		ResetColor();

		DrawText(font, 50, 145 + i * 20, 400, 145 + i * 20 + 20, D3DCOLOR_RGBA(0, 0, 0, 255), devname.c_str());
	}

	for (int i = 0; i < maxDevicesOnTheScreen; ++i)
	{
		int id = i + devPos;

		if (id < 0)
			break;

		if (id >= allDevNames.size())
			break;

		std::string devname = allDevNames[id];
		devname.resize(32);

		ResetColor();

		if (MouseInBox_WH(500, 145 + i * 20 + 1, 350, 18))
		{
			if (MouseClick())
			{
				if (allDevNames[id] != devName)
				{
					//RecorderDeviceRetrieve(i, curRecDevNum);
					//BASS_SetDevice(i);
					devName = allDevNames[id];
					devNum = id;
					devicechanged = true;
				}
			}
			else
			{
				SetColor(128, 128, 128, 255);
			}
		}

		if (curDevNum == id)
			SetColor(128, 128, 128, 255);


		DrawPicture(px1, 500, 145 + i * 20 + 1, 350, 18);

		ResetColor();

		DrawText(font, 500, 145 + i * 20, 850, 145 + i * 20 + 20, D3DCOLOR_RGBA(0, 0, 0, 255), devname.c_str());
	}

	if (devicechanged)
	{

		gaudio->SetDevice(devNum);
		gaudio->SetRecDevice(recDevNum);

		for(int i=0; i < 16; ++i)
			RecorderDeviceRetrieve(devNum, recDevNum);

		curDevNum = devNum;
		curRecDevNum = recDevNum;
		//BASS_SetDevice(devNum);
	}

	static int librarynumber = 0;

	if (DrawChecker(basslib, bassX, bassY, librarynumber == 1))
	{
		RefreshDeviceChoice();
		librarynumber = 1;
		SwitchAudioLibrary(librarynumber, mainWnd, RECBUFFERDELAYMS);
		RecorderSetCallbacks();
	}

	if (DrawChecker(portylib, portyX, portyY, librarynumber == 0))
	{
		RefreshDeviceChoice();
		librarynumber = 0;
		SwitchAudioLibrary(librarynumber, mainWnd, RECBUFFERDELAYMS);
		RecorderSetCallbacks();
	}

	/////////////////////

	static int delaymode = 0;

	if (DrawChecker(delayslow, delayslowX, delayslowY, delaymode == 1))
	{
		delaymode = 1;
		RecorderSetStretchSamples(tapeSlowdownSamples);
	}

	if (DrawChecker(delayfast, delayfastX, delayfastY, delaymode == 0))
	{
		delaymode = 0;
		RecorderSetStretchSamples(tapeSlowdownMode2Samples);
	}

	////////////////////////

	if (DrawButton(applyimg, applyX, applyY))
	{
		//for (int i = 0; i < 16; ++i)
		//{
			//HRECORD recrehash = BASS_RecordStart(48000, 1, 0, 0, 0);
			//BASS_ChannelUpdate(recrehash, 500);
			//BASS_ChannelStop(recrehash);
		//}
		//RecorderDeviceRetrieve();
		spFrame = 1;
	}
}

////////////////////////////////////////////////////////////////////////////////

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, char*, int)
{
	InitFolders();


	InitLog("errorlog.txt");
	WriteToLog("Portable Tape Recorder, log started...");

	TiXmlDocument doc;

	if ( ! doc.LoadFile("portabletaperecordersettings.xml") )
	{
		MessageBox(0, "Settings file [portabletaperecordersettings.xml] failed", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}

	TiXmlElement* elem = doc.RootElement();

	if ( ! elem->Attribute("w", &windowWidth) )
		windowWidth = 496;

	if ( ! elem->Attribute("h", &windowHeight) )
		windowHeight = 291;

	diskName = elem->Attribute("name");
	if ( ! diskName ) 
		diskName = "PortableTapeRecorder";

	diskDesc = elem->Attribute("desc");
	if ( ! diskDesc )
		diskDesc = "Portable Tape Recorder";


	///////////////////////////////////////////////////////////////

	WNDCLASS wc = { 0, ProccessWindow, 0, 0, hInst, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), 0, CreateSolidBrush(RGB(0,0,0)), 0, diskName };
	 
	if(!RegisterClass(&wc))
		return -1;

	mainWnd = CreateWindowEx(WS_EX_APPWINDOW, diskName, diskName, WS_POPUP | WS_BORDER | WS_GROUP, 0, 0, windowWidth, windowHeight, 0, 0, hInst, 0);

	//HMENU hMenu = GetSystemMenu(mainWnd, FALSE);
	//DeleteMenu(hMenu, SC_MINIMIZE, MF_BYCOMMAND);
	//DeleteMenu(hMenu, SC_MAXIMIZE, MF_BYCOMMAND);
	//DeleteMenu(hMenu, SC_RESTORE,  MF_BYCOMMAND);

	if ( ! mainWnd )
	{
		MessageBox(0, "Window failed!", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}

  RECT windowRect, clientRect;

  GetWindowRect( mainWnd, & windowRect );
  GetClientRect( mainWnd, & clientRect );

  int realWindowWidth, realWindowHeight;

  realWindowWidth = windowWidth * 2 - clientRect.right;
  realWindowHeight = windowHeight * 2 - clientRect.bottom;

  RECT desktopRect;

  GetWindowRect( GetDesktopWindow(), & desktopRect );

  //int windowLeft, windowTop; 

  windowLeft = (desktopRect.right - desktopRect.left - realWindowWidth) / 2;
  windowTop = (desktopRect.bottom - desktopRect.top - realWindowHeight) / 2;

  SetWindowPos
  (
    mainWnd, 
    GetTopWindow(0), 
    windowLeft, 
    windowTop, 
    realWindowWidth, 
    realWindowHeight, 
    SWP_SHOWWINDOW
  );

	SetCursor(LoadCursor(0,IDC_ARROW));

	SetForegroundWindow(mainWnd);

	InitNotifyIconData(hInst);

	//////////////////////////////////////////////////////////////////

	if ( ! InitD3D(mainWnd) )
	{
		MessageBox(0, "D3D failed", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}

	DrawStartupScreen();

	//////////////////////////////////////////////////////////////////

	if( ! InitInput(hInst, mainWnd) )
	{
		MessageBox(mainWnd, "DInput failed", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}

	//////////////////////////////////////////////////////

	allDevNames.reserve(16);
	allDevRecNames.reserve(16);

	// init bass with default device, at 44.1khz, default flags, main window, default class
	//if (! BASS_Init(-1, 48000, BASS_DEVICE_MONO, mainWnd, 0))
	if( !gaudio->Init(mainWnd, RECBUFFERDELAYMS) )
	{
		MessageBox(0, "Audio library failed!", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}

	//BASS_Start();

	//BASS_Start();

	TiXmlElement* temp;

	temp = elem->FirstChildElement("settings");
	
	while (temp)
	{

		const char* id = temp->Attribute("id");

		if (!id)
			break;

		if (strcmp(id, "exporttags") == 0)
		{
			exportartist = temp->Attribute("artist");
		}


		if (strcmp(id, "adcsetup") == 0)
		{
			temp->Attribute("tapeSlowdownSamples", &tapeSlowdownSamples);
			temp->Attribute("tapeSlowdownMode2Samples", &tapeSlowdownMode2Samples);
			if (tapeSlowdownSamples < 0)
				tapeSlowdownSamples = 0;
			if (tapeSlowdownSamples > 10)
				tapeSlowdownSamples = 10;
			if (tapeSlowdownMode2Samples < 0)
				tapeSlowdownMode2Samples = 0;
			if (tapeSlowdownMode2Samples > 10)
				tapeSlowdownMode2Samples = 10;
			temp->Attribute("adcIntMaxNatural", &adcIntMaxNatural);
			if (adcIntMaxNatural == 0)
				adcIntMaxNatural = 32500;
		}

		temp = temp->NextSiblingElement("settings");
	}

	RecorderSetStretchSamples(tapeSlowdownSamples);

	temp = elem->FirstChildElement("regulator");

	while (temp)
	{

		const char* id = temp->Attribute("id");

		if (!id)
			break;

		if (strcmp(id, "RQ") == 0)
		{
			temp->Attribute("defaultvalue", &regulatorDefaultValue);
			if (regulatorDefaultValue < 0.1)
				regulatorDefaultValue = 0.1;
			if (regulatorDefaultValue > 1.0)
				regulatorDefaultValue = 1.0;
			temp->Attribute("x", &regulatorX);
			temp->Attribute("y", &regulatorY);
			temp->Attribute("diameter", &regulatorDiameter);
			regulatorImage = temp->Attribute("image");
			regulatorPressImage = temp->Attribute("onPress");
			regulatorPtrImage = temp->Attribute("imagePtr");
		}

		temp = temp->NextSiblingElement("regulator");
	}


	temp = elem->FirstChildElement("button");

	while ( temp )
	{
		const char* id = temp->Attribute("id");

		if ( ! id )
			break;

		if ( strcmp(id, "quit") == 0 )
		{
			temp->Attribute("x", &quitX );
			temp->Attribute("y", &quitY );
			quitImage = temp->Attribute("image");
			quitOverImage = temp->Attribute("onMouseOver");
		}

		if ( strcmp(id, "hide") == 0 )
		{
			temp->Attribute("x", &hideX );
			temp->Attribute("y", &hideY );
			hideImage = temp->Attribute("image");
			hideOverImage = temp->Attribute("onMouseOver");
		}

		if (strcmp(id, "addriaa") == 0)
		{
			temp->Attribute("x", &addriaaX);
			temp->Attribute("y", &addriaaY);
			addriaaImage = temp->Attribute("image");
			addriaaOverImage = temp->Attribute("onMouseOver");
		}

		if (strcmp(id, "undofx") == 0)
		{
			temp->Attribute("x", &undoFXX);
			temp->Attribute("y", &undoFXY);
			undofxImage = temp->Attribute("image");
			undofxOverImage = temp->Attribute("onMouseOver");
		}

		if (strcmp(id, "apply") == 0)
		{
			temp->Attribute("x", &applyX);
			temp->Attribute("y", &applyY);
			applyImage = temp->Attribute("image");
			applyOverImage = temp->Attribute("onMouseOver");
		}


		if (strcmp(id, "settings") == 0)
		{
			temp->Attribute("x", &settingsX);
			temp->Attribute("y", &settingsY);
			settingsImage = temp->Attribute("image");
			settingsOverImage = temp->Attribute("onMouseOver");
		}

		temp = temp->NextSiblingElement("button");
	}

	temp = elem->FirstChildElement("switch");

	while(temp)
	{
		const char* id = temp->Attribute("id");

		if( ! id )
			break;

		if ( strcmp( id, "rec") == 0 )
		{
			temp->Attribute("x", &recX);
			temp->Attribute("y", &recY);
			recImage = temp->Attribute("image1");
			recOverImage = temp->Attribute("onMouseOver1");
			stopImage = temp->Attribute("image2");
			stopOverImage = temp->Attribute("onMouseOver2");
		}

		if (strcmp(id, "play") == 0)
		{
			temp->Attribute("x", &playX);
			temp->Attribute("y", &playY);
			playImage = temp->Attribute("image1");
			playOverImage = temp->Attribute("onMouseOver1");
			stopplayImage = temp->Attribute("image2");
			stopplayOverImage = temp->Attribute("onMouseOver2");
		}

		if (strcmp(id, "zoom") == 0)
		{
			temp->Attribute("x", &zoomX);
			temp->Attribute("y", &zoomY);
			zoomInImage = temp->Attribute("image1");
			zoomInOverImage = temp->Attribute("onMouseOver1");
			zoomOutImage = temp->Attribute("image2");
			zoomOutOverImage = temp->Attribute("onMouseOver2");
			zoomFullImage = temp->Attribute("image3");
			zoomFullOverImage = temp->Attribute("onMouseOver3");
		}

		if (strcmp(id, "remaster") == 0)
		{
			temp->Attribute("x", &remasterX);
			temp->Attribute("y", &remasterY);
			remasterImage = temp->Attribute("image1");
			remasterOverImage = temp->Attribute("onMouseOver1");
			remasterplayingImage = temp->Attribute("image2");
			remasterplayingOverImage = temp->Attribute("onMouseOver2");
		}

		if (strcmp(id, "normalize") == 0)
		{
			temp->Attribute("x", &normalizeX);
			temp->Attribute("y", &normalizeY);
			normalizeImage = temp->Attribute("image1");
			normalizeOverImage = temp->Attribute("onMouseOver1");
			limitImage = temp->Attribute("image2");
			limitOverImage = temp->Attribute("onMouseOver2");

		}


		if (strcmp(id, "loop") == 0)
		{
			temp->Attribute("x", &loopX);
			temp->Attribute("y", &loopY);
			loopOnImage = temp->Attribute("image1");
			loopOnOverImage = temp->Attribute("onMouseOver1");
			loopOffImage = temp->Attribute("image2");
			loopOffOverImage = temp->Attribute("onMouseOver2");
		}

		if (strcmp(id, "export") == 0)
		{
			temp->Attribute("x", &exportX);
			temp->Attribute("y", &exportY);
			exportImage = temp->Attribute("image1");
			exportOverImage = temp->Attribute("onMouseOver1");
			exportMp3Image = temp->Attribute("image2");
			exportMp3OverImage = temp->Attribute("onMouseOver2");
		}

		if (strcmp(id, "onoff") == 0)
		{
			temp->Attribute("x", &onoffX);
			temp->Attribute("y", &onoffY);
			onImage = temp->Attribute("image1");
			onOverImage = temp->Attribute("onMouseOver1");
			offImage = temp->Attribute("image2");
			offOverImage = temp->Attribute("onMouseOver2");
		}


		temp = temp->NextSiblingElement("switch");
	}

	temp = elem->FirstChildElement("meter");

	if (temp)
	{
		temp->Attribute("x", &meterX);
		temp->Attribute("y", &meterY);
		temp->Attribute("w", &meterWidth);
		temp->Attribute("h", &meterHeight);
		temp->Attribute("offsetx", &meterOffsetX);
		temp->Attribute("offsety", &meterOffsetY);

		meterImage = temp->Attribute("image");
	}

	temp = elem->FirstChildElement("track");

	if (temp)
	{
		temp->Attribute("x", &trackX);
		temp->Attribute("y", &trackY);
		temp->Attribute("w", &trackWidth);
		temp->Attribute("h", &trackHeight);
	}

	temp = elem->FirstChildElement("equalizer");

	if ( temp )
	{
		temp->Attribute("y", &equalizerY);
		temp->Attribute("cropLeft", &eqCropLeft);
		temp->Attribute("cropRight", &eqCropRight);
	}

	///////////////////////////////////////////////////////
	// 
	temp = elem->FirstChildElement("resources");

	while (temp)
	{
		const char* id = temp->Attribute("id");

		if (!id)
			break;

		if (strcmp(id, "beep") == 0)
		{
			beepSound = temp->Attribute("sound");
		}

		temp = temp->NextSiblingElement("resources");
	}

	///////////////////////////////////////////////////////
// 
	temp = elem->FirstChildElement("checker");

	while (temp)
	{
		const char* id = temp->Attribute("id");

		if (!id)
			break;

		if (strcmp(id, "basslib") == 0)
		{
			bassLibImage = temp->Attribute("image1");
			bassLibImageOn = temp->Attribute("image2");
			temp->Attribute("x", &bassX);
			temp->Attribute("y", &bassY);
		}

		if (strcmp(id, "portytracklib") == 0)
		{
			portyLibImage = temp->Attribute("image1");
			portyLibImageOn = temp->Attribute("image2");
			temp->Attribute("x", &portyX);
			temp->Attribute("y", &portyY);
		}


		if (strcmp(id, "delayslow") == 0)
		{
			delaySlowImage = temp->Attribute("image1");
			delaySlowImageOn = temp->Attribute("image2");
			temp->Attribute("x", &delayslowX);
			temp->Attribute("y", &delayslowY);
		}

		if (strcmp(id, "delayfast") == 0)
		{
			delayFastImage = temp->Attribute("image1");
			delayFastImageOn = temp->Attribute("image2");
			temp->Attribute("x", &delayfastX);
			temp->Attribute("y", &delayfastY);
		}



		temp = temp->NextSiblingElement("checker");
	}


	//////////////////////////////////////////////////////////////

	temp = elem->FirstChildElement("icon");

	if ( temp )
	{
		DeleteFile("PortableTapeRecorder.backup.exe");

		//
		std::string iconName;
		iconName = temp->Attribute("image");

		// current dir
		char currentDir[256] = { 0 };

    GetCurrentDirectory(256, currentDir);

		// current exe
		char currentExe[256] = { 0 };

    GetModuleFileName(0, currentExe, 256);

    std::string newExe;

		newExe = currentDir;
		newExe += "\\PortableTapeRecorder.backup.exe";
    
    MoveFile(currentExe, newExe.c_str());

		CopyFile(newExe.c_str(), currentExe, FALSE); 

		// code by ryadovoy
		// http://forum.sources.ru/index.php?showtopic=129351

		ResourceUpdater updater;
		updater.SetExecutableFileName(currentExe);

		std::string iconFileName; 
		iconFileName = currentDir;
		iconFileName += "/images/" + iconName;

		updater.SetIconFileName(iconFileName.c_str());

		LONG err = updater.UpdateResources();

		if ( err ) 
		{
			FILE* log;
			log = fopen("IconChangeError.txt", "w+");
			fprintf(log, "Error code: %i\n", err);
			fclose(log);
		}

	}

	//////////////////////////////////////////////////////////////

	///////////////////////////////////////////////////////////////

	back = LoadPicture("images/back.png");

	settingsback = LoadPicture("images/settings.png");

	quit[0] = LoadPicture("images/" + quitImage);
	quit[1] = LoadPicture("images/" + quitOverImage);

	hide[0] = LoadPicture("images/" + hideImage);
	hide[1] = LoadPicture("images/" + hideOverImage);

	normalize[0] = LoadPicture("images/" + normalizeImage);
	normalize[1] = LoadPicture("images/" + normalizeOverImage);

	limit[0] = LoadPicture("images/" + limitImage);
	limit[1] = LoadPicture("images/" + limitOverImage);

	exportbut[0] = LoadPicture("images/" + exportImage);
	exportbut[1] = LoadPicture("images/" + exportOverImage);

	exportmp3but[0] = LoadPicture("images/" + exportMp3Image);
	exportmp3but[1] = LoadPicture("images/" + exportMp3OverImage);

	addriaa[0] = LoadPicture("images/" + addriaaImage);
	addriaa[1] = LoadPicture("images/" + addriaaOverImage);

	undofx[0] = LoadPicture("images/" + undofxImage);
	undofx[1] = LoadPicture("images/" + undofxOverImage);

	rec[0] = LoadPicture("images/" + recImage);
	rec[1] = LoadPicture("images/" + recOverImage);

	play[0] = LoadPicture("images/" + playImage);
	play[1] = LoadPicture("images/" + playOverImage);

	stopplay[0] = LoadPicture("images/" + stopplayImage);
	stopplay[1] = LoadPicture("images/" + stopplayOverImage);

	zoomIn[0] = LoadPicture("images/" + zoomInImage);
	zoomIn[1] = LoadPicture("images/" + zoomInOverImage);

	zoomOut[0] = LoadPicture("images/" + zoomOutImage);
	zoomOut[1] = LoadPicture("images/" + zoomOutOverImage);

	zoomFull[0] = LoadPicture("images/" + zoomFullImage);
	zoomFull[1] = LoadPicture("images/" + zoomFullOverImage);

	remaster[0] = LoadPicture("images/" + remasterImage);
	remaster[1] = LoadPicture("images/" + remasterOverImage);

	remasterPlaying[0] = LoadPicture("images/" + remasterplayingImage);
	remasterPlaying[1] = LoadPicture("images/" + remasterplayingOverImage);

	meter = LoadPicture("images/" + meterImage);

	stop[0] = LoadPicture("images/" + stopImage);
	stop[1] = LoadPicture("images/" + stopOverImage);

	applyimg[0] = LoadPicture("images/" + applyImage);
	applyimg[1] = LoadPicture("images/" + applyOverImage);

	settingsimg[0] = LoadPicture("images/" + settingsImage);
	settingsimg[1] = LoadPicture("images/" + settingsOverImage);

	loopon[0] = LoadPicture("images/" + loopOnImage);
	loopon[1] = LoadPicture("images/" + loopOnOverImage);

	loopoff[0] = LoadPicture("images/" + loopOffImage);
	loopoff[1] = LoadPicture("images/" + loopOffOverImage);

	onimg[0] = LoadPicture("images/" + onImage);
	onimg[1] = LoadPicture("images/" + onOverImage);

	offimg[0] = LoadPicture("images/" + offImage);
	offimg[1] = LoadPicture("images/" + offOverImage);

	regulatorimg = LoadPicture("images/" + regulatorImage);
	regulatorpressimg = LoadPicture("images/" + regulatorPressImage);
	regulatorptrimg = LoadPicture("images/" + regulatorPtrImage);

	arrowup = LoadPicture("images/arrowup.png");
	arrowdown = LoadPicture("images/arrowdown.png");

	basslib[0] = LoadPicture("images/" + bassLibImage);
	basslib[1] = LoadPicture("images/" + bassLibImageOn);

	portylib[0] = LoadPicture("images/" + portyLibImage);
	portylib[1] = LoadPicture("images/" + portyLibImageOn);

	delayslow[0] = LoadPicture("images/" + delaySlowImage);
	delayslow[1] = LoadPicture("images/" + delaySlowImageOn);

	delayfast[0] = LoadPicture("images/" + delayFastImage);
	delayfast[1] = LoadPicture("images/" + delayFastImageOn);


	px1 = LoadPicture("images/1px.png");

	///////////////////////////////////////////////////////////////
	
	DWORD bassSpectrumLength;

	if ( windowWidth < 512 ) 
	{
		spectrumLength = 512;
		bassSpectrumLength = BASS_DATA_FFT1024;
	}
	else
	if ( windowWidth < 1024 )
	{
		spectrumLength = 1024;
		bassSpectrumLength = BASS_DATA_FFT2048;
	}
	else
	if( windowWidth < 2048 ) 
	{
		spectrumLength = 2048;
		bassSpectrumLength = BASS_DATA_FFT4096;
	}

	ZeroMemory(spectrum, sizeof(spectrum));
	ZeroMemory(pointsX, sizeof(pointsX));
	ZeroMemory(pointsY, sizeof(pointsY));
	ZeroMemory(prevPointsY, sizeof(prevPointsY));

	//////////////////////////////////

	beepforme = LoadSound("sounds/" + beepSound);

	if ( beepforme != -1 )
	{
		RecorderLoadBeepSound(sounds[beepforme].sampledata, sounds[beepforme].length);
	}

	//WriteToLog("Sound len: %i", sounds[beepforme].length);

	///////////////////////////////////////////////////////////////

	/*if (!BASS_RecordInit(-1))
	{
		MessageBox(0, "Recording failed!", "Sorry!", MB_ICONERROR | MB_OK);
		return -1;
	}*/

	/*if (!BASS_SetConfig(BASS_CONFIG_BUFFER, 250))
	{
		WriteToLog("BASS SetConfig at app init failed.");
	}*/
	
	/*if (!BASS_SetConfig(BASS_CONFIG_REC_BUFFER, RECBUFFERDELAYMS))
	{
		WriteToLog("BASS SetConfig at app init failed.");
	}*/

	InitRec(baserec);

	////////////////////////////////////////////////////////////////

	DeviceCheck();

	//static DWORD bassdevice = BASS_GetDevice();
	
	//static DWORD bassrecorddevice = BASS_RecordGetDevice();

	BuildResourceLink("precompiledresources/resources.cpp");

	///////////////////////////////////////////////////////////////
 
	MSG msg = { 0 };

	while(msg.message != WM_QUIT) 
	{
		if(PeekMessage(&msg, 0, 0, 0, 0)) 
		{
			if(GetMessage(&msg, 0, 0, 0)) 
			{
				TranslateMessage(&msg);
				DispatchMessage (&msg);
			}
		} 
		else 
		{
			DWORD time0 = timeGetTime();

			UpdateInput(mainWnd);
			
			RecorderDebugCheck();

			if (IsRecorderWriting())
				normalize_mode = 1;

			d3dd->Clear(0,0, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER, 0xFF000000, 0, 0);
			d3dd->BeginScene();

			DrawPicture(back, 0, 0, windowWidth, windowHeight);
			
			static int n = 0;
			
			disableWindowMovement = false;

			noButtonIsOver = true;

			//
			uint transitionAlpha = 255;

			ResetColor(); // set alpha back

			//
			//spFrame = 1;//( spFrame + 1 ) % 2;
			//

			if (spFrame == 1)
			{
				DrawMainScreen();
			}
			else if (spFrame == 2)
			{
				//RecorderDeviceRetrieve(-1, -1);
				DrawSettingsScreen();
			}
		
			/////////////////////////////////////////////////////////////
			
			/*
			does work
			if (KeyPressed(DIK_R))
			{
				RecorderDeviceRetrieve();
			}*/

			// window move

			static int mouseDownTime = 0;
			static int mouseRemX = 0;
			static int mouseRemY = 0;

			if( MouseL() && noButtonIsOver && !disableWindowMovement)
			{
				bool rem = false;

				if ( mouseDownTime < 200 )
				{
					rem = true;
				}

				mouseDownTime += timeDt;

				if( mouseDownTime > 200 )
				{					
					if ( rem ) 
					{
						mouseRemX = mouseX;
						mouseRemY = mouseY;
						rem = false;
					}

					windowLeft = cursorX - mouseRemX;
					windowTop = cursorY - mouseRemY;

					SetWindowPos
					(
						mainWnd, 
						GetTopWindow(0), 
						windowLeft, 
						windowTop, 
						0, 
						0, 
						SWP_NOSIZE | SWP_SHOWWINDOW 
					);

				}
			}
			else
			{
				mouseDownTime = 0;
			}

#ifdef DEBUGSCREEN
			DrawText(font, 55, 55, 155, 80, D3DCOLOR_RGBA(255, 255, 255, 255), "FPS: %0.3f sec", timeFps);
#endif 

			SetBlendNormal();
			ResetColor();

			d3dd->EndScene();
			if(d3dd->Present(0, 0, 0, 0) == D3DERR_DEVICELOST ) {
			#ifndef _DEBUG
				PostQuitMessage(0);
			#endif
			}

			//
			if (KeyPressed(DIK_ESCAPE))
			{
				PostQuitMessage(0);
			}

			DWORD error = gaudio->GetError();

			if (error != 0)
			{
				WriteToLog("Audio Library error at the cycle: %i", error);

				if (!RecorderDeviceRetrieve(curDevNum, curRecDevNum))
				{
					WriteToLog("Failed to retrieve the device with the current device choice");
					if (!RecorderDeviceRetrieve(-1, -1))
					{
						WriteToLog("Failed to retrieve the device with the default device choice");
						static char errmessage[256];
						sprintf(errmessage, "Device error: %i and failed to retrieve!", error);
						MessageBox(mainWnd, errmessage, "Error", MB_OK);
						PostQuitMessage(0);
					}
				}
			}

			//
			WriteRec();

			std::this_thread::sleep_for(std::chrono::microseconds(6));

			// calc time

			float deltaInSeconds;
			

			timeDt = timeGetTime() - time0;
			deltaInSeconds = (float) timeDt / 1000.0f; 
			timeFps = (1.0f / deltaInSeconds);

		  float sleep = ( 1.0f / timeMaximumFps ) - deltaInSeconds;

			 if(sleep > 0.0f )//&& IsRecorderIdling())
			 {
				Sleep( sleep * 1000.0f);			
			 }
			 /*else
			 {
				 //int mcssleep = (int)(sleep * 1000000.0f);
				 std::this_thread::sleep_for(std::chrono::microseconds(2500));
			 }*/

			// recalc after sleep

			timeDt = timeGetTime() - time0;
			deltaInSeconds = (float) timeDt / 1000.0f; 
			timeFps = (1.0f / deltaInSeconds);
			
			timeTicks += timeDt;

			RecorderUpdate(deltaInSeconds);
		}
	}

	if( !IsWindowVisible( mainWnd ) )
  {
    Shell_NotifyIcon(NIM_DELETE, &notifyIconData);
  }
	
	//SaveRec();

	FreeSounds();

	gaudio->Free();

	DestroyWindow(mainWnd);

	CloseLog();

	return 0;
}
