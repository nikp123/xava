#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include <tchar.h>
#include <windows.h>
#include <windowsx.h>
#include <dwmapi.h>

// it must be in this OCD breaking order or it will fail to compile ;(
#ifdef GL
    #include "output/shared/gl/glew.h"
    #include <GL/wglext.h>
#endif

#ifdef CAIRO
    #include <cairo.h>
    #include <cairo-win32.h>
    #include "output/shared/cairo/main.h"
#endif

#include "output/shared/graphical.h"
#include "shared.h"

#include "main.h"

const char szAppName[] = "XAVA";
const char wcWndName[] = "XAVA";

HWND xavaWinWindow;
MSG xavaWinEvent;
HMODULE xavaWinModule;
WNDCLASSEX xavaWinClass;
HDC xavaWinFrame;
TIMECAPS xavaPeriod;

// These hold the size and position of the window if you're switching to fullscreen mode
// because Windows (or rather WIN32) doesn't do it internally
i32 oldX, oldY, oldW, oldH;

#ifdef GL
    BOOL WINAPI wglSwapIntervalEXT (int interval);
    HGLRC xavaWinGLFrame;
#endif

#ifdef CAIRO
    xava_cairo_handle *xavaCairoHandle;
    cairo_surface_t   *xavaCairoSurface;
#endif

// a crappy workaround for a flawed event-loop design
static bool resized=FALSE, quit=FALSE;

// Warning: shitty win32 api design below, cover your poor eyes
// Why can't I just pass this shit immediately to my events function
// instead of doing this shit

// Retarded shit, exhibit A:
void *xavaHandleForWindowFuncBecauseWinAPIIsOutdated;
LRESULT CALLBACK WindowFunc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    // this shit is beyond retarded
    XAVA *xava = xavaHandleForWindowFuncBecauseWinAPIIsOutdated;

    // god why
    if(xava == NULL)
        return DefWindowProc(hWnd,msg,wParam,lParam);

    XAVA_CONFIG *conf = &xava->conf;

    switch(msg) {
        case WM_CREATE:
            return XAVA_RESIZE;
        case WM_KEYDOWN:
            switch(wParam) {
                // should_reload = 1
                // resizeTerminal = 2
                // bail = -1
                case 'A':
                    conf->bs++;
                    return XAVA_RESIZE;
                case 'S':
                    if(conf->bs > 0) conf->bs--;
                    return XAVA_RESIZE;
                case 'F': // fullscreen
                    conf->flag.fullscreen = !conf->flag.fullscreen;
                    return XAVA_RESIZE;
                case VK_UP:
                    conf->sens *= 1.05;
                    break;
                case VK_DOWN:
                    conf->sens *= 0.95;
                    break;
                case VK_LEFT:
                    conf->bw++;
                    return XAVA_RESIZE;
                case VK_RIGHT:
                    if (conf->bw > 1) conf->bw--;
                    return XAVA_RESIZE;
                case 'R': //reload config
                    return XAVA_RELOAD;
                case 'Q':
                    return XAVA_QUIT;
                case VK_ESCAPE:
                    return XAVA_QUIT;
                case 'B':
                    conf->bgcol = (rand()<<16)|rand();
                    return XAVA_REDRAW;
                case 'C':
                    if(conf->gradients) break;
                    conf->col = (rand()<<16)|rand();
                    return XAVA_REDRAW;
                default: break;
            }
            break;
        case WM_SIZE:
            calculate_win_geo(xava, LOWORD(lParam), HIWORD(lParam));
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

EXP_FUNC void xavaOutputClear(XAVA *xava) {
    #ifdef GL
        GLClear(xava);
    #endif
    #ifdef CAIRO
        UNUSED(xava);
        __internal_xava_output_cairo_clear(xavaCairoHandle);
    #endif
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

EXP_FUNC int xavaInitOutput(XAVA *xava) {
    XAVA_CONFIG *conf = &xava->conf;

    // reset event trackers
    resized=FALSE;
    quit=FALSE;

    // never assume that memory is clean
    oldX = 0; oldY = 0; oldW = -1; oldH = -1;

    // get handle
    xavaWinModule = GetModuleHandle(NULL);

    // register window class
    xavaBailCondition(!register_window_win(xavaWinModule), "RegisterClassEx failed");

    // get window size etc..
    int screenWidth, screenHeight;
    GetDesktopResolution(&screenWidth, &screenHeight);

    // adjust window position etc...
    calculate_win_pos(xava, screenWidth, screenHeight,
            conf->w, conf->h);

    // why?
    //if(!conf->transF) conf->interactF=1; // correct practicality error

    // extended and standard window styles
    DWORD dwExStyle=0, dwStyle=0;
    if(conf->flag.transparency) dwExStyle|=WS_EX_TRANSPARENT;
    if(!conf->flag.interact) dwExStyle|=WS_EX_LAYERED;
    if(!conf->flag.taskbar) dwExStyle|=WS_EX_TOOLWINDOW;
    if(conf->flag.border) dwStyle|=WS_CAPTION;

    // create window
    xavaWinWindow = CreateWindowEx(dwExStyle, szAppName, wcWndName,
        WS_POPUP | WS_VISIBLE | dwStyle,
        xava->outer.x, xava->outer.y, xava->outer.w, xava->outer.h,
        NULL, NULL, xavaWinModule, NULL);
    xavaBailCondition(!xavaWinWindow, "CreateWindowEx failed");

    // transparency fix
    if(conf->flag.transparency) SetLayeredWindowAttributes(xavaWinWindow,
            0x00FFFFFF, 255, LWA_ALPHA | LWA_COLORKEY);
    SetWindowPos(xavaWinWindow, conf->flag.beneath ? HWND_BOTTOM : HWND_NOTOPMOST,
            xava->outer.x, xava->outer.y,
            xava->outer.w, xava->outer.h,
            SWP_SHOWWINDOW);

    // we need the desktop window manager to enable transparent background (from Vista ...onward)
    DWM_BLURBEHIND bb = {0};
    HRGN hRgn = CreateRectRgn(0, 0, -1, -1);
    bb.dwFlags = DWM_BB_ENABLE | DWM_BB_BLURREGION;
    bb.hRgnBlur = hRgn;
    bb.fEnable = conf->flag.transparency;
    bb.fTransitionOnMaximized = true;
    DwmEnableBlurBehindWindow(xavaWinWindow, &bb);

    xavaWinFrame = GetDC(xavaWinWindow);

    #ifdef GL
        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR),
            1,                                // Version Number
            PFD_DRAW_TO_WINDOW      |         // Format Must Support Window
        #ifdef GL
            PFD_SUPPORT_OPENGL      |         // Format Must Support OpenGL
        #endif
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
        xavaBailCondition(PixelFormat == 0, "ChoosePixelFormat failed!");

        BOOL bResult = SetPixelFormat(xavaWinFrame, PixelFormat, &pfd);
        xavaBailCondition(bResult == FALSE, "SetPixelFormat failed!");

        xavaWinGLFrame = wglCreateContext(xavaWinFrame);
        xavaBailCondition(xavaWinGLFrame == NULL, "wglCreateContext failed!");
    #endif

    // process colors
    if(!strcmp(conf->color, "default")) {
        // we'll just get the accent color (which is way easier and an better thing to do)
        WINBOOL opaque = 1;
        DWORD fancyVariable;
        HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
        conf->col = fancyVariable;
        xavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
    } // as for the other case, we don't have to do any more processing

    if(!strcmp(conf->bcolor, "default")) {
        // we'll just get the accent color (which is way easier and a better thing to do)
        WINBOOL opaque = 1;
        DWORD fancyVariable;
        HRESULT error = DwmGetColorizationColor(&fancyVariable, &opaque);
        conf->bgcol = fancyVariable;
        xavaWarnCondition(!SUCCEEDED(error), "DwmGetColorizationColor failed");
    }

    // WGL
    #ifdef GL
        wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);
        GLInit(xava);
    #endif
    #ifdef CAIRO
        xavaCairoSurface = cairo_win32_surface_create(xavaWinFrame);
        __internal_xava_output_cairo_init(xavaCairoHandle,
                cairo_create(xavaCairoSurface));
    #endif

    // set up precise timers (otherwise unstable framerate)
    xavaWarnCondition(timeGetDevCaps(&xavaPeriod, sizeof(TIMECAPS))!=MMSYSERR_NOERROR,
            "Unable to obtain precise system timers! Stability may be of concern!");

    timeEndPeriod(0);
    timeBeginPeriod(xavaPeriod.wPeriodMin);

    return 0;
}

EXP_FUNC int xavaOutputApply(XAVA *xava) {
    XAVA_CONFIG *conf = &xava->conf;

    //ReleaseDC(xavaWinWindow, xavaWinFrame);

    if(conf->flag.fullscreen) {
        POINT Point = {0};
        HMONITOR Monitor = MonitorFromPoint(Point, MONITOR_DEFAULTTONEAREST);
        MONITORINFO MonitorInfo = { 0 };
        if (GetMonitorInfo(Monitor, &MonitorInfo)) {
            DWORD Style = WS_POPUP | WS_VISIBLE;
            SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

            // dont overwrite old size on accident if already fullscreen
            if(oldW == -1 && oldH == -1) {
                oldX = xava->outer.x; oldY = xava->outer.y;
                oldW = xava->outer.w; oldH = xava->outer.h;
            }

            // resizing to full screen
            SetWindowPos(xavaWinWindow, 0, MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                MonitorInfo.rcMonitor.right-MonitorInfo.rcMonitor.left,
                MonitorInfo.rcMonitor.bottom-MonitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
        }
        // check if the window has been already resized
    } else if(oldW != -1 && oldH != -1) {
        xava->outer.x = oldX; xava->outer.y = oldY;
        calculate_win_geo(xava, oldW, oldH);

        oldW = -1;
        oldH = -1;

        // restore window properties
        DWORD Style = WS_POPUP | WS_VISIBLE | (conf->flag.border? WS_CAPTION:0);
        SetWindowLongPtr(xavaWinWindow, GWL_STYLE, Style);

        SetWindowPos(xavaWinWindow, 0,
                xava->outer.w, xava->outer.y,
                xava->outer.w, xava->outer.h,
                SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    }

    xavaOutputClear(xava);

    // WGL stuff
    #ifdef GL
        GLApply(xava);
        PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT = (PFNWGLSWAPINTERVALEXTPROC)(void*)wglGetProcAddress("wglSwapIntervalEXT");
        wglSwapIntervalEXT(conf->vsync);
    #endif
    #ifdef CAIRO
        __internal_xava_output_cairo_apply(xavaCairoHandle);
    #endif

    return 0;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(XAVA *xava) {
    // don't even fucking ask
    xavaHandleForWindowFuncBecauseWinAPIIsOutdated = xava;

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

    XG_EVENT_STACK *eventStack = 
    #if defined(CAIRO)
        __internal_xava_output_cairo_event(xavaCairoHandle);
    #elif defined(GL)
        GLEvent(xava);
    #endif

    while(pendingXAVAEventStack(eventStack)) {
        XG_EVENT event = popXAVAEventStack(eventStack);
        if(event != XAVA_IGNORE)
            return event;
    }

    return XAVA_IGNORE;
}

EXP_FUNC void xavaOutputDraw(XAVA *xava) {
    #ifdef GL
        wglMakeCurrent(xavaWinFrame, xavaWinGLFrame);
        GLDraw(xava);
        SwapBuffers(xavaWinFrame);
    #endif

    #ifdef CAIRO
        UNUSED(xava);
        __internal_xava_output_cairo_draw(xavaCairoHandle);
        SwapBuffers(xavaWinFrame);
    #endif
}

EXP_FUNC void xavaOutputCleanup(XAVA *xava) {
    #ifdef GL
        GLCleanup(xava);
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(xavaWinGLFrame);
    #endif

    #ifdef CAIRO
        UNUSED(xava);
        __internal_xava_output_cairo_cleanup(xavaCairoHandle);
    #endif

    // Normal Win32 stuff
    timeEndPeriod(xavaPeriod.wPeriodMin);
    ReleaseDC(xavaWinWindow, xavaWinFrame);
    DestroyWindow(xavaWinWindow);
    UnregisterClass(szAppName, xavaWinModule);
    //CloseHandle(xavaWinModule);
}

EXP_FUNC void xavaOutputLoadConfig(XAVA *xava) {
    XAVA_CONFIG *conf = &xava->conf;

    #ifdef GL
        // VSync is a must due to shit Windows timers
        conf->vsync = 1;
        GLConfigLoad(xava);
    #endif
    #ifdef CAIRO
        conf->vsync = 0;
        xavaCairoHandle = __internal_xava_output_cairo_load_config(xava);
    #endif
}

