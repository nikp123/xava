#include <stdio.h>
#include <time.h>
#include "graphical_win.h"
#include "graphical.h"
#include "../config.h"

const char szAppName[] = "XAVA";
const char wcWndName[] = "XAVA";

HWND xavaWinWindow;
MSG xavaWinEvent;
HMODULE xavaWinModule;
WNDCLASSEX xavaWinClass;	// same thing as window classes in Xlib
HDC xavaWinFrame;
HGLRC xavaWinGLFrame;
TIMECAPS xavaPeriod;
unsigned int *gradientColor;

static double glColors[8];
static double gradColors[24];

BOOL WINAPI wglSwapIntervalEXT (int interval);

// a crappy workaround for a flawed event design
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
					//p.fullF = !p.fullF;
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

void clear_screen_win() {
	glColors[0] = ARGB_R_32(p.col)/255.0;
	glColors[1] = ARGB_G_32(p.col)/255.0;
	glColors[2] = ARGB_B_32(p.col)/255.0;
	glColors[3] = p.transF ? p.foreground_opacity : 1.0;
	glColors[4] = ARGB_A_32(p.shdw_col)/255.0;
	glColors[5] = ARGB_R_32(p.shdw_col)/255.0;
	glColors[6] = ARGB_G_32(p.shdw_col)/255.0;
	glColors[7] = ARGB_B_32(p.shdw_col)/255.0;

	for(int i=0; i<p.gradients; i++) {
		gradColors[i*3] = ARGB_R_32(gradientColor[i])/255.0;
		gradColors[i*3+1] = ARGB_G_32(gradientColor[i])/255.0;
		gradColors[i*3+2] = ARGB_B_32(gradientColor[i])/255.0;;
	}

	//glClearColor(ARGB_R_32(p.bgcol)/255.0, ARGB_G_32(p.bgcol)/255.0, ARGB_B_32(p.bgcol)/255.0, 0.0);
}

unsigned char register_window_win(HINSTANCE HIn) {
	xavaWinClass.cbSize=sizeof(WNDCLASSEX);
	xavaWinClass.style=CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	xavaWinClass.lpfnWndProc=WindowFunc;
	xavaWinClass.cbClsExtra=0;
	xavaWinClass.cbWndExtra=0;
	xavaWinClass.hInstance=HIn;
	xavaWinClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	xavaWinClass.hIcon=LoadIcon(NULL,IDI_APPLICATION);
	xavaWinClass.hCursor=LoadCursor(NULL,IDC_ARROW);
	xavaWinClass.hbrBackground=(HBRUSH)CreateSolidBrush(0x00000000);
	xavaWinClass.lpszMenuName=NULL;
	xavaWinClass.lpszClassName=szAppName;
	xavaWinClass.hIconSm=LoadIcon(NULL,IDI_APPLICATION);

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

void init_opengl_win() {
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
	glClearColor(0, 0, 0, 0);
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

void resize_framebuffer() {
	glViewport(0, 0, p.w, p.h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0, p.w, 0, p.h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

int init_window_win() {

	// reset event trackers
	resized=FALSE;
	quit=FALSE;

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
	if(!strcmp(p.winA, "top")){
		p.wx = (screenWidth - p.w) / 2 + p.wx;
	}else if(!strcmp(p.winA, "bottom")){
		p.wx = (screenWidth - p.w) / 2 + p.wx;
		p.wy = (screenHeight - p.h) + (-1*p.wy);
	}else if(!strcmp(p.winA, "top_left")){
		// Nothing to do here :P
	}else if(!strcmp(p.winA, "top_right")){
		p.wx = (screenHeight - p.w) + (-1*p.wx);
	}else if(!strcmp(p.winA, "left")){
		p.wy = (screenHeight - p.h) / 2;
	}else if(!strcmp(p.winA, "right")){
		p.wx = (screenWidth - p.w) + (-1*p.wx);
		p.wy = (screenHeight - p.h) / 2 + p.wy;
	}else if(!strcmp(p.winA, "bottom_left")){
		p.wy = (screenHeight - p.h) + (-1*p.wy);
	}else if(!strcmp(p.winA, "bottom_right")){
		p.wx = (screenWidth - p.w) + (-1*p.wx);
		p.wy = (screenHeight - p.h) + (-1*p.wy);
	}else if(!strcmp(p.winA, "center")){
		p.wx = (screenWidth - p.w) / 2 + p.wx;
		p.wy = (screenHeight - p.h) / 2 + p.wy;
	}
	// Some error checking
#ifdef DEBUG
	if(p.wx > screenWidth - p.w) printf("Warning: Screen out of bounds (X axis)!");
	if(p.wy > screenHeight - p.h) printf("Warning: Screen out of bounds (Y axis)!");
#endif

	if(!p.transF) p.interactF=1; // correct practicality error

	// create window
	xavaWinWindow = CreateWindowEx((p.transF?WS_EX_TRANSPARENT:0)|(p.interactF?0:WS_EX_LAYERED|WS_EX_COMPOSITED), 
		szAppName, wcWndName, WS_POPUP | WS_VISIBLE | (p.borderF?WS_CAPTION:0), p.wx, p.wy, p.w, p.h, 
		NULL, NULL, xavaWinModule, NULL);
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
		// instead of messing around with average colors like on Xlib
		// we'll just get the accent color (which is way easier and an better thing to do)

		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p.col = fancyVariable;
		if(!SUCCEEDED(error)) {
			MessageBox(NULL, "DwmGetColorizationColor - failed", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
	} else if(p.color[0] != '#')
		p.col = definedColors[p.col];
	else sscanf(p.color, "#%x", &p.col);

	if(!strcmp(p.bcolor, "default")) {
		// instead of messing around with average colors like on Xlib
		// we'll just get the accent color (which is way easier and a better thing to do)

		WINBOOL opaque = 1;
		DWORD fancyVariable;
		HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
		p.bgcol = fancyVariable;
		if(!SUCCEEDED(error)) {
			MessageBox(NULL, "DwmGetColorizationColor - failed", "Error", MB_OK | MB_ICONERROR);
			return 1;
		}
	} else if(p.bcolor[0] != '#') 
		p.bgcol = definedColors[p.bgcol];
	else sscanf(p.bcolor, "#%x", &p.bgcol);

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

	return 0;
}

void apply_win_settings() {
	clear_screen_win();
	resize_framebuffer(p.w, p.h);
	//ReleaseDC(xavaWinWindow, xavaWinFrame);

	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)wglGetProcAddress("wglSwapIntervalEXT"); 
	wglSwapIntervalEXT(p.vsync);
	return;
}

int get_window_input_win() {
	while(PeekMessage(&xavaWinEvent, xavaWinWindow, 0, 0, PM_REMOVE)) {
		TranslateMessage(&xavaWinEvent);
		int r=DispatchMessage(&xavaWinEvent);  // handle return values
		
		// so you may have wondered why do i do stuff like this
		// it's because non-keyboard/mouse messages DONT pass through a return values
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

void draw_graphical_win(int bars, int rest, int f[200]) {
	wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);

	// clear color and calculate pixel witdh in double
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if(drawGLBars(rest, bars, glColors, gradColors, f)) exit(EXIT_FAILURE);

	// dumb workarounds for dumb OSes
	glBegin(GL_QUADS);
		glColor4d(ARGB_R_32(p.bgcol)/255.0, ARGB_G_32(p.bgcol)/255.0, ARGB_B_32(p.bgcol)/255.0, p.background_opacity);
		glVertex2d(0.0, 0.0);
		glVertex2d(0.0, p.h);
		glVertex2d(p.w, p.h);
		glVertex2d(p.w, 0.0);
	glEnd();

	glFlush();

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
	CloseHandle(xavaWinModule);
}
