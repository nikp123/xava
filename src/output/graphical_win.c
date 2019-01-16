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
unsigned int *gradientColor;

LRESULT CALLBACK WindowFunc(HWND hWnd,UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: break;
	case WM_DESTROY: 
 		// Perform cleanup tasks.
		PostQuitMessage(0); 
    break; 
        default:
            return DefWindowProc(hWnd,msg,wParam,lParam);
    }

    return 0;
}

unsigned char register_window_win(HINSTANCE HIn) {
    xavaWinClass.cbSize=sizeof(WNDCLASSEX);
    xavaWinClass.style=CS_HREDRAW | CS_VREDRAW;
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

	if(transparentFlag) {
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

   HDC hdc = GetDC(hWnd);
   int PixelFormat = ChoosePixelFormat(hdc, &pfd);
   if (PixelFormat == 0) {
      assert(0);
      return FALSE ;
   }

   BOOL bResult = SetPixelFormat(hdc, PixelFormat, &pfd);
   if (bResult==FALSE) {
      assert(0);
      return FALSE ;
   }

   xavaWinGLFrame = wglCreateContext(hdc);
   if (!xavaWinGLFrame){
      assert(0);
      return FALSE;
   }

   ReleaseDC(hWnd, hdc);

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
	if(!strcmp(windowAlignment, "top")){
		windowX = (screenWidth - p.w) / 2 + windowX;
	}else if(!strcmp(windowAlignment, "bottom")){
		windowX = (screenWidth - p.w) / 2 + windowX;
		windowY = (screenHeight - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "top_left")){
		// Nothing to do here :P
	}else if(!strcmp(windowAlignment, "top_right")){
		windowX = (screenHeight - p.w) + (-1*windowX);
	}else if(!strcmp(windowAlignment, "left")){
		windowY = (screenHeight - p.h) / 2;
	}else if(!strcmp(windowAlignment, "right")){
		windowX = (screenWidth - p.w) + (-1*windowX);
		windowY = (screenHeight - p.h) / 2 + windowY;
	}else if(!strcmp(windowAlignment, "bottom_left")){
		windowY = (screenHeight - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "bottom_right")){
		windowX = (screenWidth - p.w) + (-1*windowX);
		windowY = (screenHeight - p.h) + (-1*windowY);
	}else if(!strcmp(windowAlignment, "center")){
		windowX = (screenWidth - p.w) / 2 + windowX;
		windowY = (screenHeight - p.h) / 2 + windowY;
	}
	// Some error checking
	#ifdef DEBUG
		if(windowX > screenWidth - p.w) printf("Warning: Screen out of bounds (X axis)!");
		if(windowY > screenHeight - p.h) printf("Warning: Screen out of bounds (Y axis)!");
	#endif
	
	// create window
	xavaWinWindow = CreateWindowEx(interactable ? WS_EX_APPWINDOW : WS_EX_COMPOSITED | WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, szAppName, wcWndName, WS_VISIBLE | WS_POPUP, windowX, windowY, p.w, p.h, NULL, NULL, xavaWinModule, NULL);
	if(xavaWinWindow == NULL) {
		MessageBox(NULL, "CreateWindowEx - failed", "Error", MB_OK | MB_ICONERROR);
		return 1;
	}
	// transparency fix
	SetLayeredWindowAttributes(xavaWinWindow, 0, 255, LWA_ALPHA);
	SetWindowPos(xavaWinWindow, keepInBottom ? HWND_BOTTOM : HWND_TOPMOST, windowX, windowY, p.w, p.h, SWP_SHOWWINDOW);

	
	// we need the desktop window manager to enable transparent background (from Vista ...onward)
	DWM_BLURBEHIND bb = {0};
	HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
	bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
	bb.hRgnBlur = hRgn;
	bb.fEnable = transparentFlag;
	DwmEnableBlurBehindWindow(xavaWinWindow, &bb);
	
	CreateHGLRC(xavaWinWindow);
	xavaWinFrame = GetDC(xavaWinWindow);
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
	gradientColor = malloc(sizeof(int)*p.gradient_count);
	for(int i=0; i<p.gradient_count; i++)
		sscanf(p.gradient_colors[i], "#%06x", &gradientColor[i]);
	
	// set up opengl and stuff
	init_opengl_win();
	return 0;
}

void apply_win_settings() {
	resize_framebuffer(p.w, p.h);
	ReleaseDC(xavaWinWindow, xavaWinFrame);

	if(!transparentFlag) glClearColor(((p.bgcol>>16)%256)/255.0, ((p.bgcol>>8)%256)/255.0,(p.bgcol%256)/255.0, 0.0f);
	PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = wglGetProcAddress("wglSwapIntervalEXT"); 
	wglSwapIntervalEXT(p.framerate);
	return;
}

int get_window_input_win() {
	while(PeekMessage(&xavaWinEvent, NULL, WM_KEYFIRST, WM_MOUSELAST, PM_REMOVE)) {	
		TranslateMessage(&xavaWinEvent);
		DispatchMessage(&xavaWinEvent);	// windows handles the rest
		switch(xavaWinEvent.message) {
			case WM_KEYDOWN:
				switch(xavaWinEvent.wParam) {
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
						//fs = !fs;
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
						if(transparentFlag) break;
						p.bgcol = (rand()<<16)+rand();
						return 3;
					case 'C':
						if(p.gradient) break;
						p.col = (rand()<<16)+rand();
						return 3;
			    default: break;
				}
			  break;
			case WM_CLOSE:
			 	return -1;
			case WM_DESTROY:
				return -1;
			case WM_QUIT:
				return -1;
			case WM_SIZE:
			{
				RECT rect;
				if(GetWindowRect(xavaWinWindow, &rect)) {
					p.w = rect.right - rect.left;
					p.h = rect.bottom - rect.top;
				}
				return 2;
			}
		}
	}
	return 0;
}

void draw_graphical_win(int bars, int rest, int f[200]) {
	HDC hdc = GetDC(xavaWinWindow);
        wglMakeCurrent(hdc, xavaWinGLFrame);

	// clear color and calculate pixel witdh in double
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
	double glColors[8] = {((p.col>>16)%256)/255.0, ((p.col>>8)%256)/255.0, (p.col%256)/255.0, p.foreground_opacity, ((unsigned int)p.shadow_color>>24)%256/255.0,
		 ((unsigned int)p.shadow_color>>16)%256/255.0, ((unsigned int)p.shadow_color>>8)%256/255.0, (unsigned int)p.shadow_color%256/255.0};
	double gradColors[24] = {0.0};
	for(int i=0; i<p.gradient_count; i++) {
		gradColors[i*3] = ((gradientColor[i]>>16)%256)/255.0;
		gradColors[i*3+1] = ((gradientColor[i]>>8)%256)/255.0;
		gradColors[i*3+2] = (gradientColor[i]%256)/255.0;;
	}

	if(drawGLBars(rest, bars, glColors, gradColors, f)) exit(EXIT_FAILURE);	
	glFlush();

	// swap buffers	
	SwapBuffers(hdc);
	ReleaseDC(xavaWinWindow, hdc);
}

void cleanup_graphical_win(void) {
	free(gradientColor);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(xavaWinGLFrame);
	ReleaseDC(xavaWinWindow, xavaWinFrame);
	DestroyWindow(xavaWinWindow);
	UnregisterClass(szAppName, xavaWinModule);	
	CloseHandle(xavaWinModule);
}
