#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

// it must be in this OCD breaking order or it will fail to compile ;(
#include "../shared/gl.h"
#include <GL/wglext.h>

#include "../graphical.h"
#include "../../shared.h"

#include "main.h"

const char szAppName[] = "XAVA";
const char wcWndName[] = "XAVA";

HWND xavaWinWindow;
MSG xavaWinEvent;
HMODULE xavaWinModule;
WNDCLASSEX xavaWinClass;
HDC xavaWinFrame;
HGLRC xavaWinGLFrame;
TIMECAPS xavaPeriod;

// These hold the size and position of the window if you're switching to fullscreen mode
// because Windows (or rather WIN32) doesn't do it internally
DWORD oldX, oldY, oldW, oldH;

BOOL WINAPI wglSwapIntervalEXT (int interval);

// a crappy workaround for a flawed event-loop design
static _Bool resized=FALSE, quit=FALSE;

// Warning: shitty win32 api design below, cover your poor eyes
// Why can't I just pass this shit immediately to my events function
// instead of doing this shit

// Retarded shit, exhibit A:
void *xavaHandleForWindowFuncBecauseWinAPIIsOutdated;
LRESULT CALLBACK WindowFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

	// this shit is beyond retarded
	struct XAVA_HANDLE *xava = xavaHandleForWindowFuncBecauseWinAPIIsOutdated;

	// god why
	if(xava == NULL)
		return DefWindowProc(hWnd,msg,wParam,lParam);

	struct config_params *p = &xava->conf;

	switch(msg) {
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			switch(wParam) {
				// should_reload = 1
				// resizeTerminal = 2
				// bail = -1
				case 'A':
					p->bs++;
					return XAVA_RESIZE;
				case 'S':
					if(p->bs > 0) p->bs--;
					return XAVA_RESIZE;
				case 'F': // fullscreen
					p->fullF = !p->fullF;
					return XAVA_RESIZE;
				case VK_UP:
					p->sens *= 1.05;
					break;
				case VK_DOWN:
					p->sens *= 0.95;
					break;
				case VK_LEFT:
					p->bw++;
					return XAVA_RESIZE;
				case VK_RIGHT:
					if (p->bw > 1) p->bw--;
					return XAVA_RESIZE;
				case 'R': //reload config
					return XAVA_RELOAD;
				case 'Q':
					return XAVA_QUIT;
				case VK_ESCAPE:
					return XAVA_QUIT;
				case 'B':
					p->bgcol = (rand()<<16)|rand();
					return XAVA_REDRAW;
				case 'C':
					if(p->gradients) break;
					p->col = (rand()<<16)|rand();
					return XAVA_REDRAW;
				default: break;
			}
			break;
		case WM_SIZE:
			calculate_inner_win_pos(xava, LOWORD(lParam), HIWORD(lParam));
			resized=TRUE;
			return XAVA_RELOAD;
		case WM_CLOSE:
			// Perform cleanup tasks.
			PostQuitMessage(0);
			quit=TRUE;
			return XAVA_QUIT;
		case WM_DESTROY:
			quit=TRUE;
			return XAVA_QUIT;
		case WM_QUIT: // lol why is this ignored
			break;
		case WM_NCHITTEST: {
			LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
			if (hit == HTCLIENT) hit = HTCAPTION;
			return hit;
		}
		default:
			return DefWindowProc(hWnd,msg,wParam,lParam);
	}
	return XAVA_IGNORE;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand) {
	GLClear(hand);
}

unsigned char register_window_win(HINSTANCE HIn) {
	xavaWinClass.cbSize=sizeof(WNDCLASSEX);
	xavaWinClass.style=CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	xavaWinClass.lpfnWndProc=WindowFunc;
	xavaWinClass.cbClsExtra=0;
	xavaWinClass.cbWndExtra=0;
	xavaWinClass.hInstance=HIn;
	xavaWinClass.hIcon=NULL;
	xavaWinClass.hIconSm=xavaWinClass.hIcon;
	xavaWinClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	xavaWinClass.hbrBackground=(HBRUSH)CreateSolidBrush(0x00000000);
	xavaWinClass.lpszMenuName=NULL;
	xavaWinClass.lpszClassName=szAppName;

	return RegisterClassEx(&xavaWinClass);
}

void GetDesktopResolution(int *horizontal, int *vertical) {
	RECT desktop;

	// Get a handle to the desktop window
	const HWND hDesktop = GetDesktopWindow();
	// Get the size of screen to the variable desktop
	GetWindowRect(hDesktop, &desktop);

	// return dimensions
	(*horizontal) = desktop.right;
	(*vertical) = desktop.bottom;

	return;
}

unsigned char CreateHGLRC(HWND hWnd) {
	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),
		1,                                // Version Number
		PFD_DRAW_TO_WINDOW      |         // Format Must Support Window
		PFD_SUPPORT_OPENGL      |         // Format Must Support OpenGL
		PFD_SUPPORT_COMPOSITION |         // Format Must Support Composition
		PFD_DOUBLEBUFFER,                 // Must Support Double Buffering
		PFD_TYPE_RGBA,                    // Request An RGBA Format
		32,                               // Select Our Color Depth
		0, 0, 0, 0, 0, 0,                 // Color Bits Ignored
		8,                                // An Alpha Buffer
		0,                                // Shift Bit Ignored
		0,                                // No Accumulation Buffer
		0, 0, 0, 0,                       // Accumulation Bits Ignored
		24,                               // 16Bit Z-Buffer (Depth Buffer)
		8,                                // Some Stencil Buffer
		0,                                // No Auxiliary Buffer
		PFD_MAIN_PLANE,                   // Main Drawing Layer
		0,                                // Reserved
		0, 0, 0                           // Layer Masks Ignored
	};

	int PixelFormat = ChoosePixelFormat(xavaWinFrame, &pfd);
	if (PixelFormat == 0) {
		assert(0);
		return FALSE ;
	}

	BOOL bResult = SetPixelFormat(xavaWinFrame, PixelFormat, &pfd);
	if (bResult==FALSE) {
		assert(0);
		return FALSE ;
	}

	xavaWinGLFrame = wglCreateContext(xavaWinFrame);
	if (!xavaWinGLFrame){
		assert(0);
		return FALSE;
	}

	return TRUE;
}

EXP_FUNC int xavaInitOutput(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// reset event trackers
	resized=FALSE;
	quit=FALSE;

	// never assume that memory is clean
	oldX = 0; oldY = 0; oldW = 0; oldH = 0;

	// get handle
	xavaWinModule = GetModuleHandle(NULL);

	// register window class
	xavaBailCondition(!register_window_win(xavaWinModule), "RegisterClassEx failed");

	// get window size etc..
	int screenWidth, screenHeight;
	GetDesktopResolution(&screenWidth, &screenHeight);

	// adjust window position etc...
	calculate_win_pos(p, screenWidth, screenHeight);

	// why?
	//if(!p->transF) p->interactF=1; // correct practicality error

	// extended and standard window styles
	DWORD dwExStyle=0, dwStyle=0;
	if(p->transF) dwExStyle|=WS_EX_TRANSPARENT;
	if(!p->interactF) dwExStyle|=WS_EX_LAYERED|WS_EX_COMPOSITED;
	if(!p->taskbarF) dwExStyle|=WS_EX_TOOLWINDOW;
	if(p->borderF) dwStyle|=WS_CAPTION;

	// create window
	xavaWinWindow = CreateWindowEx(dwExStyle, szAppName, wcWndName, WS_POPUP | WS_VISIBLE | dwStyle,
		p->wx, p->wy, p->w, p->h, NULL, NULL, xavaWinModule, NULL);
	xavaBailCondition(!xavaWinWindow, "CreateWindowEx failed");

	// transparency fix
	if(p->transF) SetLayeredWindowAttributes(xavaWinWindow, 0, 255, LWA_ALPHA);
	SetWindowPos(xavaWinWindow, p->bottomF ? HWND_BOTTOM : HWND_NOTOPMOST, p->wx, p->wy, p->w, p->h, SWP_SHOWWINDOW);

	// we need the desktop window manager to enable transparent background (from Vista ...onward)
	DWM_BLURBEHIND bb = {0};
	HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.hRgnBlur = hRgn;
	bb.fEnable = p->transF;
	DwmEnableBlurBehindWindow(xavaWinWindow, &bb);

	xavaWinFrame = GetDC(xavaWinWindow);
	CreateHGLRC(xavaWinWindow);

	// process colors
	if(!strcmp(p->color, "default")) {
		// we'll just get the accent color (which is way easier and an better thing to do)
		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p->col = fancyVariable;
		xavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
	} // as for the other case, we don't have to do any more processing

	if(!strcmp(p->bcolor, "default")) {
		// we'll just get the accent color (which is way easier and a better thing to do)
		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p->bgcol = fancyVariable;
		xavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
	}

	// WGL
	wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);
	GLInit(hand);

	// set up precise timers (otherwise unstable framerate)
	xavaWarnCondition(timeGetDevCaps(&xavaPeriod, sizeof(TIMECAPS))!=MMSYSERR_NOERROR,
			"Unable to obtain precise system timers! Stability may be of concern!");

	timeEndPeriod(0);
	timeBeginPeriod(xavaPeriod.wPeriodMin);

	return 0;
}

EXP_FUNC int xavaOutputApply(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	//ReleaseDC(xavaWinWindow, xavaWinFrame);

	if(p->fullF) {
		POINT Point = {0};
		HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetMonitorInfo(Monitor, &MonitorInfo)) {
			DWORD Style = WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

			// dont overwrite old size on accident if already fullscreen
			if(!(oldX||oldH||oldX||oldY)) {
				oldX = p->wx; oldY = p->wy;
				oldW = hand->w; oldH = hand->h;
			}

			// resizing to full screen
			SetWindowPos(xavaWinWindow, 0, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right-MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom-MonitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}
		// check if the window has been already resized
	} else if(oldW||oldH||oldX||oldY) {
		p->wx = oldX; p->wy = oldY;
		calculate_inner_win_pos(hand, oldW, oldH);

		// reset to default if restoring
		oldX = 0; oldY = 0;
		oldW = 0; oldH = 0;

		// restore window properties
		DWORD Style = WS_POPUP | WS_VISIBLE | (p->borderF?WS_CAPTION:0);
		SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

		SetWindowPos(xavaWinWindow, 0, p->wx, p->wy, hand->w, hand->h,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	xavaOutputClear(hand);

	// WGL stuff
	GLApply(hand);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT");
	wglSwapIntervalEXT(p->vsync);

	return 0;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(struct XAVA_HANDLE *hand) {
	// don't even fucking ask
	xavaHandleForWindowFuncBecauseWinAPIIsOutdated = hand;

	while(PeekMessage(&xavaWinEvent, xavaWinWindow, 0, 0, PM_REMOVE)) {
		TranslateMessage(&xavaWinEvent);

		// you fucking piece of shit why are you predefined type AAAAAAAAAAAAAA
		int r=DispatchMessage(&xavaWinEvent);  // handle return values

		// so you may have wondered why do i do stuff like this
		// it's because non-keyboard/mouse messages DONT pass through return values
		// which, guess what, completely breaks my previous design - thanks micro$oft, really appreciate it

		if(quit) {
			quit=FALSE;
			return XAVA_QUIT;
		}
		if(resized) {
			resized=FALSE;
			return XAVA_RESIZE;
		}
		if(r != XAVA_IGNORE)
			return r;
	}
	return XAVA_IGNORE;
}

EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *hand) {
	// WGL
	wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);
	GLDraw(hand);
	SwapBuffers(xavaWinFrame);
}

EXP_FUNC void xavaOutputCleanup(struct XAVA_HANDLE *hand) {
	// WGL
	GLCleanup();
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(xavaWinGLFrame);

	// Normal Win32 stuff
	timeEndPeriod(xavaPeriod.wPeriodMin);
	ReleaseDC(xavaWinWindow, xavaWinFrame);
	DestroyWindow(xavaWinWindow);
	UnregisterClass(szAppName, xavaWinModule);
	//CloseHandle(xavaWinModule);
}

EXP_FUNC void xavaOutputHandleConfiguration(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// VSync is a must due to shit Windows timers
	p->vsync = 1;

	GLShadersLoad(hand);
}

