#include <stdio.h>
#include <time.h>
#include "graphical_win.h"
#include "graphical.h"
#include "../config.h"

#define WIN_ICON_PATH "xava.ico"

const char szAppName[] = "XAVA";
const char wcWndName[] = "XAVA";

HWND xavaWinWindow;
MSG xavaWinEvent;
HMODULE xavaWinModule;
WNDCLASSEX xavaWinClass;
HDC xavaWinFrame;
HGLRC xavaWinGLFrame;
TIMECAPS xavaPeriod;
unsigned int *gradientColor;
unsigned int shadowSize;

static float glColors[8];
static float gradColors[24];

// These hold the size and position of the window if you're switching to fullscreen mode
// because Windows (or rather WIN32) doesn't do it internally
DWORD oldX, oldY, oldW, oldH;

BOOL WINAPI wglSwapIntervalEXT (int interval);

// a crappy workaround for a flawed event-loop design
static _Bool resized=FALSE, quit=FALSE;

LRESULT CALLBACK WindowFunc(HWND hWnd,UINT msg, WPARAM wParam, LPARAM lParam) {
	switch(msg) {
		case WM_CREATE:
			break;
		case WM_KEYDOWN:
			switch(wParam) {
				// should_reload = 1
				// resizeTerminal = 2
				// bail = -1
				case 'A':
					p.bs++;
					return 2;
				case 'S':
					if(p.bs > 0) p.bs--;
					return 2;
				case 'F': // fullscreen
					p.fullF = !p.fullF;
					return 2;
				case VK_UP:
					p.sens *= 1.05;
					break;
				case VK_DOWN:
					p.sens *= 0.95;
					break;
				case VK_LEFT:
					p.bw++;
					return 2;
				case VK_RIGHT:
					if (p.bw > 1) p.bw--;
					return 2;
				case 'R': //reload config
					return 1;
				case 'Q':
					return -1;
				case VK_ESCAPE:
					return -1;
				case 'B':
					p.bgcol = (rand()<<16)|rand();
					return 3;
				case 'C':
					if(p.gradients) break;
					p.col = (rand()<<16)|rand();
					return 3;
				default: break;
			}
			break;
		case WM_SIZE:
			p.w=LOWORD(lParam);
			p.h=HIWORD(lParam);
			resized=TRUE;
			return 2;
		case WM_CLOSE:
			// Perform cleanup tasks.
			PostQuitMessage(0); 
			quit=TRUE;
			return -1;
		case WM_DESTROY:
			quit=TRUE;
			return -1;
		case WM_QUIT:
			break;
		case WM_NCHITTEST: {
			LRESULT hit = DefWindowProc(hWnd, msg, wParam, lParam);
			if (hit == HTCLIENT) hit = HTCAPTION;
			return hit;
		}
		default:
			return DefWindowProc(hWnd,msg,wParam,lParam);
	}
	return 0;
}

void clear_screen_win(void) {
	glColors[0] = ARGB_R_32(p.col)/255.0;
	glColors[1] = ARGB_G_32(p.col)/255.0;
	glColors[2] = ARGB_B_32(p.col)/255.0;
	glColors[3] = p.transF ? p.foreground_opacity : 1.0;

	glColors[4] = ARGB_R_32(p.shdw_col)/255.0;
	glColors[5] = ARGB_G_32(p.shdw_col)/255.0;
	glColors[6] = ARGB_B_32(p.shdw_col)/255.0;
	glColors[7] = ARGB_A_32(p.shdw_col)/255.0;

	glColors[8] = ARGB_R_32(p.bgcol)/255.0;
	glColors[9] = ARGB_G_32(p.bgcol)/255.0;
	glColors[10] = ARGB_B_32(p.bgcol)/255.0;
	glColors[11] = p.transF ? p.background_opacity : 1.0;

	for(int i=0; i<p.gradients; i++) {
		gradColors[i*3] = ARGB_R_32(gradientColor[i])/255.0;
		gradColors[i*3+1] = ARGB_G_32(gradientColor[i])/255.0;
		gradColors[i*3+2] = ARGB_B_32(gradientColor[i])/255.0;;
	}
}

unsigned char register_window_win(HINSTANCE HIn) {
	xavaWinClass.cbSize=sizeof(WNDCLASSEX);
	xavaWinClass.style=CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	xavaWinClass.lpfnWndProc=WindowFunc;
	xavaWinClass.cbClsExtra=0;
	xavaWinClass.cbWndExtra=0;
	xavaWinClass.hInstance=HIn;
	xavaWinClass.hIcon=LoadImage( // returns a HANDLE so we have to cast to HICON
  NULL,             // hInstance must be NULL when loading from a file
  WIN_ICON_PATH,    // the icon file name
  IMAGE_ICON,       // specifies that the file is an icon
  0,                // width of the image (we'll specify default later on)
  0,                // height of the image
  LR_LOADFROMFILE|  // we want to load a file (as opposed to a resource)
  LR_DEFAULTSIZE|   // default metrics based on the type (IMAGE_ICON, 32x32)
  LR_SHARED         // let the system release the handle when it's no longer used
);
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

void init_opengl_win(void) {
	VBOGLsetup();

	glEnable(GL_ALPHA_TEST);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	if(p.transF) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
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

void resize_framebuffer(void) {
	glViewport(0, 0, p.w, p.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, p.w, 0, p.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int init_window_win(void) {
	// reset event trackers
	resized=FALSE;
	quit=FALSE;

	// never assume that memory is clean
	oldX = 0; oldY = 0; oldW = 0; oldH = 0;

	// get handle
	xavaWinModule = GetModuleHandle(NULL);
	FreeConsole();

	// register window class
	if(!register_window_win(xavaWinModule)) {
		MessageBox(NULL, "RegisterClassEx - failed", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}

	// get window size etc..
	int screenWidth, screenHeight;
	GetDesktopResolution(&screenWidth, &screenHeight);

	// adjust window position etc...
	calculate_win_pos(&p.wx, &p.wy, p.w, p.h, screenWidth, screenHeight, p.winA);

	// Some error checking
#ifdef DEBUG
	if(p.wx > screenWidth - p.w) printf("Warning: Screen out of bounds (X axis)!");
	if(p.wy > screenHeight - p.h) printf("Warning: Screen out of bounds (Y axis)!");
#endif

	if(!p.transF) p.interactF=1; // correct practicality error

	// extended and standard window styles
	DWORD dwExStyle=0, dwStyle=0;
	if(p.transF) dwExStyle|=WS_EX_TRANSPARENT;
	if(!p.interactF) dwExStyle|=WS_EX_LAYERED|WS_EX_COMPOSITED;
	if(!p.taskbarF) dwExStyle|=WS_EX_TOOLWINDOW;
	if(p.borderF) dwStyle|=WS_CAPTION;
	// create window
	xavaWinWindow = CreateWindowEx(dwExStyle, szAppName, wcWndName, WS_POPUP | WS_VISIBLE | dwStyle,
		p.wx, p.wy, p.w, p.h, NULL, NULL, xavaWinModule, NULL);
	if(xavaWinWindow == NULL) {
		MessageBox(NULL, "CreateWindowEx - failed", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	// transparency fix
	if(p.transF) SetLayeredWindowAttributes(xavaWinWindow, 0, 255, LWA_ALPHA);
	SetWindowPos(xavaWinWindow, p.bottomF ? HWND_BOTTOM : HWND_NOTOPMOST, p.wx, p.wy, p.w, p.h, SWP_SHOWWINDOW);


	// we need the desktop window manager to enable transparent background (from Vista ...onward)
	DWM_BLURBEHIND bb = {0};
	HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.hRgnBlur = hRgn;
	bb.fEnable = p.transF;
	DwmEnableBlurBehindWindow(xavaWinWindow, &bb);

	xavaWinFrame = GetDC(xavaWinWindow);
	CreateHGLRC(xavaWinWindow);
	wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);

	// process colors
	if(!strcmp(p.color, "default")) {
		// we'll just get the accent color (which is way easier and an better thing to do)
		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p.col = fancyVariable;
		if(!SUCCEEDED(error)) {
			MessageBox(NULL, "DwmGetColorizationColor - failed", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
	} // as for the other case, we don't have to do any more processing

	if(!strcmp(p.bcolor, "default")) {
		// we'll just get the accent color (which is way easier and a better thing to do)
		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p.bgcol = fancyVariable;
		if(!SUCCEEDED(error)) {
			MessageBox(NULL, "DwmGetColorizationColor - failed", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
	}

	// parse all of the values
	gradientColor = malloc(sizeof(int)*p.gradients);
	for(int i=0; i<p.gradients; i++)
		sscanf(p.gradient_colors[i], "#%06x", &gradientColor[i]);

	// set up opengl and stuff
	init_opengl_win();

	// set up precise timers (otherwise unstable framerate)
	if(timeGetDevCaps(&xavaPeriod, sizeof(TIMECAPS))!=MMSYSERR_NOERROR) {
		MessageBox(NULL, "Failed setting up precise timers", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	timeEndPeriod(0);
	timeBeginPeriod(xavaPeriod.wPeriodMin);

	shadowSize = p.shdw;

	return 0;
}

int apply_win_settings(void) {

	//ReleaseDC(xavaWinWindow, xavaWinFrame);

	if(p.fullF) {
		POINT Point = {0};
		HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetMonitorInfo(Monitor, &MonitorInfo)) {
			DWORD Style = WS_POPUP | WS_VISIBLE;
			SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

			// dont overwrite old size on accident if already fullscreen
			if(!(oldX||oldH||oldX||oldY)) {
				oldX = p.wx; oldY = p.wy;
				oldW = p.w; oldH = p.h;
			}

			// resizing to full screen
			SetWindowPos(xavaWinWindow, 0, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right-MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom-MonitorInfo.rcMonitor.top,
				SWP_FRAMECHANGED | SWP_SHOWWINDOW);
		}
		// check if the window has been already resized
	} else if(oldW||oldH||oldX||oldY) {
		p.wx = oldX; p.wy = oldY;
		p.w = oldW; p.h = oldH;

		// reset to default if restoring
		oldX = 0; oldY = 0;
		oldW = 0; oldH = 0;

		// restore window properties
		DWORD Style = WS_POPUP | WS_VISIBLE | (p.borderF?WS_CAPTION:0);
		SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

		SetWindowPos(xavaWinWindow, 0, p.wx, p.wy, p.w, p.h,
			SWP_FRAMECHANGED | SWP_SHOWWINDOW);
	}

	clear_screen_win();
	resize_framebuffer();

	// TODO: find a better solution, original issue:
	// transparent polys draw over opaque ones on windows
	if(shadowSize > p.bw)
		p.shdw = p.bw;

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"); 
	wglSwapIntervalEXT(p.vsync);
	return 0;
}

int get_window_input_win(void) {
	while(PeekMessage(&xavaWinEvent, xavaWinWindow, 0, 0, PM_REMOVE)) {
		TranslateMessage(&xavaWinEvent);
		int r=DispatchMessage(&xavaWinEvent);  // handle return values
		
		// so you may have wondered why do i do stuff like this
		// it's because non-keyboard/mouse messages DONT pass through return values
		// which, guess what, completely breaks my previous design - thanks micro$oft, really appreciate it
		
		if(quit) {
			quit=FALSE;
			return -1;
		}
		if(resized) {
			resized=FALSE;
			return 2;
		}
		if(r) return r;
	}
	return 0;
}

void draw_graphical_win(int bars, int rest, int f[200], int flastd[200]) {
	wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);

	// clear color and calculate pixel witdh in double
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(drawGLBars(rest, bars, glColors, gradColors, f)) exit(EXIT_FAILURE);

	// dumb workarounds for dumb OSes
	//glBegin(GL_QUADS);
	//	glColor4d(ARGB_R_32(p.bgcol)/255.0, ARGB_G_32(p.bgcol)/255.0, ARGB_B_32(p.bgcol)/255.0, p.background_opacity);
	//	glVertex2d(0.0, 0.0);
	//	glVertex2d(0.0, p.h);
	//	glVertex2d(p.w, p.h);
	//	glVertex2d(p.w, 0.0);
	//glEnd();

	//glFlush();

	// swap buffers
	SwapBuffers(xavaWinFrame);
}

void cleanup_graphical_win(void) {
	timeEndPeriod(xavaPeriod.wPeriodMin);
	free(gradientColor);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(xavaWinGLFrame);
	ReleaseDC(xavaWinWindow, xavaWinFrame);
	DestroyWindow(xavaWinWindow);
	UnregisterClass(szAppName, xavaWinModule);
	//CloseHandle(xavaWinModule);
}
