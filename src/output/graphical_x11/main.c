#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/Xrandr.h>
#ifdef GLX
#include <X11/extensions/Xrender.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../graphical.h"
#include "../../config.h"
#include "../../shared.h"
#include "main.h"

// random magic values go here
#define SCREEN_CONNECTED 0

static Pixmap gradientBox = 0;
static XColor xbgcol, xcol, *xgrad;

static XEvent xavaXEvent;
static Colormap xavaXColormap;
static Display *xavaXDisplay;
static Screen *xavaXScreen;
static Window xavaXWindow, xavaXRoot;
static GC xavaXGraphics;

static XVisualInfo xavaVInfo;
static XSetWindowAttributes xavaAttr;
static Atom wm_delete_window, wmState, fullScreen, mwmHintsProperty, wmStateBelow, taskbar;
static XClassHint xavaXClassHint;
static XWMHints xavaXWMHints;
static XEvent xev;
static Bool xavaSupportsRR;
static int xavaRREventBase;

static XRRScreenResources *xavaXScreenResources;
static XRROutputInfo      *xavaXOutputInfo;
static XRRCrtcInfo        *xavaXCrtcInfo;

static char  *monitorName;
static _Bool rootWindowEnabled;
static _Bool overrideRedirectEnabled;
static _Bool reloadOnDisplayConfigure;

#ifdef GLX
static int VisData[] = {
	GLX_RENDER_TYPE, GLX_RGBA_BIT,
	GLX_DRAWABLE_TYPE, GLX_WINDOW_BIT,
	GLX_DOUBLEBUFFER, True,
	GLX_RED_SIZE, 8,
	GLX_GREEN_SIZE, 8,
	GLX_BLUE_SIZE, 8,
	GLX_ALPHA_SIZE, 8,
	GLX_DEPTH_SIZE, 16,
	None
};
static double glColors[8], gradColors[24];
static XRenderPictFormat *pict_format;

static GLXFBConfig *fbconfigs, fbconfig;
static int numfbconfigs;

void glXSwapIntervalEXT (Display *dpy, GLXDrawable drawable, int interval);
#endif


// mwmHints helps us comunicate with the window manager
struct mwmHints {
	unsigned long flags;
	unsigned long functions;
	unsigned long decorations;
	long input_mode;
	unsigned long status;
};

static int xavaXScreenNumber;

// Some window manager definitions
enum {
	MWM_HINTS_FUNCTIONS = (1L << 0),
	MWM_HINTS_DECORATIONS =  (1L << 1),

	MWM_FUNC_ALL = (1L << 0),
	MWM_FUNC_RESIZE = (1L << 1),
	MWM_FUNC_MOVE = (1L << 2),
	MWM_FUNC_MINIMIZE = (1L << 3),
	MWM_FUNC_MAXIMIZE = (1L << 4),
	MWM_FUNC_CLOSE = (1L << 5)
};
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2


#ifdef GLX
int XGLInit(struct config_params *p) {
	// we will use the existing VisualInfo for this, because I'm not messing around with FBConfigs
	xavaGLXContext = glXCreateContext(xavaXDisplay, &xavaVInfo, NULL, 1);
	glXMakeCurrent(xavaXDisplay, xavaXWindow, xavaGLXContext);
	if(p->transF) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	return 0;
}
#endif

// Pull from the terminal colors, and afterwards, do the usual
void snatchColor(char *name, char *colorStr, int colorNum, XColor *colorObj,
		char *databaseName, XrmDatabase *xavaXResDB) {
	XrmValue value;
	char *type;
	if(!strcmp(colorStr, "default")) {
		if(databaseName) {
			if(XrmGetResource(*xavaXResDB, name, NULL, &type, &value))
				XParseColor(xavaXDisplay, xavaXColormap, value.addr, colorObj);
		} else {
			char tempColorStr[8];
			sprintf(tempColorStr, "#%06x", colorNum);
			XParseColor(xavaXDisplay, xavaXColormap, tempColorStr, colorObj);
		}
	} else {
		char tempColorStr[8];
		sprintf(tempColorStr, "#%06x", colorNum);
		XParseColor(xavaXDisplay, xavaXColormap, tempColorStr, colorObj);
	}
	XAllocColor(xavaXDisplay, xavaXColormap, colorObj);
}

void calculateColors(struct config_params *p) {
	XrmInitialize();
	XrmDatabase xavaXResDB = NULL;
	char *databaseName = XResourceManagerString(xavaXDisplay);
	if(databaseName) {
		xavaXResDB = XrmGetStringDatabase(databaseName);
	}
	snatchColor("color5", p->color, p->col, &xcol, databaseName, &xavaXResDB);
	snatchColor("color4", p->bcolor, p->bgcol, &xbgcol, databaseName, &xavaXResDB);
}

EXP_FUNC int xavaInitOutput(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// NVIDIA CPU cap utilization in Vsync fix
	setenv("__GL_YIELD", "USLEEP", 0);

	// workarounds go above if they're required to run before anything else

	// connect to the X server
	xavaXDisplay = XOpenDisplay(NULL);
	xavaBailCondition(!xavaXDisplay, "Could not find X11 display");

	xavaXScreen = DefaultScreenOfDisplay(xavaXDisplay);
	xavaXScreenNumber = DefaultScreen(xavaXDisplay);
	xavaXRoot = RootWindow(xavaXDisplay, xavaXScreenNumber);

	// select appropriate screen
	xavaXScreenResources = XRRGetScreenResources(xavaXDisplay, 
		DefaultRootWindow(xavaXDisplay));
	char *screenname = NULL; // potential bugfix if X server has no displays

	xavaSpam("Number of detected screens: %d", xavaXScreenResources->noutput);

	for(int i = 0; i < xavaXScreenResources->noutput; i++) {
		int screenwidth, screenheight, screenx, screeny;

		xavaXOutputInfo = XRRGetOutputInfo(xavaXDisplay, xavaXScreenResources,
			xavaXScreenResources->outputs[i]);

		if(xavaXOutputInfo->connection != SCREEN_CONNECTED)
			continue;

		// If the current mode is above the number of modes supported, that means
		// that THAT display is actually just disabled, so we ignore it
		if(xavaXScreenResources->ncrtc > xavaXOutputInfo->ncrtc)
			continue;

		xavaXCrtcInfo = XRRGetCrtcInfo(xavaXDisplay, xavaXScreenResources,
			xavaXOutputInfo->crtc);

		screenwidth  = xavaXCrtcInfo->width;
		screenheight = xavaXCrtcInfo->height;
		screenx      = xavaXCrtcInfo->x;
		screeny      = xavaXCrtcInfo->y;
		screenname   = strdup(xavaXOutputInfo->name);

		XRRFreeCrtcInfo(xavaXCrtcInfo);
		XRRFreeOutputInfo(xavaXOutputInfo);

		if(!strcmp(screenname, monitorName)) {
			calculate_win_pos(p, screenwidth, screenheight);

			// correct window offsets
			p->wx += screenx;
			p->wy += screeny;
			break;
		} else {
			free(screenname);
			// magic value for no screen found
			screenname = NULL;
		}
	}
	XRRFreeScreenResources(xavaXScreenResources);

	// in case that no screen matches, just use default behavior
	if(screenname == NULL) {
		calculate_win_pos(p, xavaXScreen->width, xavaXScreen->height);
	}

	// 32 bit color means alpha channel support
	#ifdef GLX
	if(p->transF) {
		fbconfigs = glXChooseFBConfig(xavaXDisplay, xavaXScreenNumber, VisData, &numfbconfigs);
		fbconfig = 0;
		for(int i = 0; i<numfbconfigs; i++) {
			XVisualInfo *visInfo = glXGetVisualFromFBConfig(xavaXDisplay, fbconfigs[i]);
			if(!visInfo) continue;
			else xavaVInfo = *visInfo;

			pict_format = XRenderFindVisualFormat(xavaXDisplay, xavaVInfo.visual);
			if(!pict_format) continue;

			fbconfig = fbconfigs[i];

			if(pict_format->direct.alphaMask > 0) break;
		}
	} else
	#endif
		XMatchVisualInfo(xavaXDisplay, xavaXScreenNumber, p->transF ? 32 : 24, TrueColor, &xavaVInfo);

	xavaAttr.colormap = XCreateColormap(xavaXDisplay, DefaultRootWindow(xavaXDisplay), xavaVInfo.visual, AllocNone);
	xavaXColormap = xavaAttr.colormap;
	calculateColors(p);
	xavaAttr.background_pixel = p->transF ? 0 : xbgcol.pixel;
	xavaAttr.border_pixel = xcol.pixel;

	xavaAttr.backing_store = Always;
	// make it so that the window CANNOT be reordered by the WM
	xavaAttr.override_redirect = overrideRedirectEnabled;

	int xavaWindowFlags = CWOverrideRedirect | CWBackingStore |  CWEventMask | CWColormap | CWBorderPixel | CWBackPixel;

	if(rootWindowEnabled) xavaXWindow = xavaXRoot;
	else xavaXWindow = XCreateWindow(xavaXDisplay, xavaXRoot, p->wx, p->wy, (unsigned int)p->w,
		(unsigned int)p->h, 0, xavaVInfo.depth, InputOutput, xavaVInfo.visual, xavaWindowFlags, &xavaAttr);
	XStoreName(xavaXDisplay, xavaXWindow, "XAVA");

	// The "X" button is handled by the window manager and not Xorg, so we set up a Atom
	wm_delete_window = XInternAtom (xavaXDisplay, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(xavaXDisplay, xavaXWindow, &wm_delete_window, 1);

	xavaXClassHint.res_name = (char *)"XAVA";
	//xavaXWMHints.flags = InputHint | StateHint;
	//xavaXWMHints.initial_state = NormalState;
	xavaXClassHint.res_class = (char *)"XAVA";
	XmbSetWMProperties(xavaXDisplay, xavaXWindow, NULL, NULL, NULL, 0, NULL, &xavaXWMHints, &xavaXClassHint);

	XSelectInput(xavaXDisplay, xavaXWindow, RRScreenChangeNotifyMask | VisibilityChangeMask | StructureNotifyMask | ExposureMask | KeyPressMask | KeymapNotify);

	#ifdef GLX
		if(XGLInit(p)) return 1;
	#endif

	XMapWindow(xavaXDisplay, xavaXWindow);
	xavaXGraphics = XCreateGC(xavaXDisplay, xavaXWindow, 0, 0);

	if(p->gradients) {
		xgrad = malloc((p->gradients+1)*sizeof(XColor));
		XParseColor(xavaXDisplay, xavaXColormap, p->gradient_colors[0], &xgrad[p->gradients]);
		XAllocColor(xavaXDisplay, xavaXColormap, &xgrad[p->gradients]);
		for(unsigned int i=0; i<p->gradients; i++) {
			XParseColor(xavaXDisplay, xavaXColormap, p->gradient_colors[i], &xgrad[i]);
			XAllocColor(xavaXDisplay, xavaXColormap, &xgrad[i]);
		}
	}

	// Set up atoms
	wmState = XInternAtom(xavaXDisplay, "_NET_WM_STATE", 0);
	taskbar = XInternAtom(xavaXDisplay, "_NET_WM_STATE_SKIP_TASKBAR", 0);
	fullScreen = XInternAtom(xavaXDisplay, "_NET_WM_STATE_FULLSCREEN", 0);
	mwmHintsProperty = XInternAtom(xavaXDisplay, "_MOTIF_WM_HINTS", 0);
	wmStateBelow = XInternAtom(xavaXDisplay, "_NET_WM_STATE_BELOW", 1);

	/**
	 * Set up window properties through Atoms,
	 * 
	 * XSendEvent requests the following format:
	 *  window  = the respective client window
	 *  message_type = _NET_WM_STATE
	 *  format = 32
	 *  data.l[0] = the action, as listed below
	 *  data.l[1] = first property to alter
	 *  data.l[2] = second property to alter
	 *  data.l[3] = source indication (0-unk,1-normal app,2-pager)
	 *  other data.l[] elements = 0
	**/
	xev.xclient.type = ClientMessage;
	xev.xclient.window = xavaXWindow;
	xev.xclient.message_type = wmState;
	xev.xclient.format = 32;
	xev.xclient.data.l[2] = 0;
	xev.xclient.data.l[3] = 0;
	xev.xclient.data.l[4] = 0;

	// keep window in bottom property
	xev.xclient.data.l[0] = p->bottomF ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xev.xclient.data.l[1] = wmStateBelow;
	XSendEvent(xavaXDisplay, xavaXRoot, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	if(p->bottomF) XLowerWindow(xavaXDisplay, xavaXWindow);

	// remove window from taskbar
	xev.xclient.data.l[0] = p->taskbarF ? _NET_WM_STATE_REMOVE : _NET_WM_STATE_ADD;
	xev.xclient.data.l[1] = taskbar;
	XSendEvent(xavaXDisplay, xavaXRoot, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);

	// Setting window options
	struct mwmHints hints;
	hints.flags = (1L << 1);
	hints.decorations = p->borderF;
	XChangeProperty(xavaXDisplay, xavaXWindow, mwmHintsProperty, mwmHintsProperty, 32, PropModeReplace, (unsigned char *)&hints, 5);

	// move the window in case it didn't by default
	XWindowAttributes xwa;
	XGetWindowAttributes(xavaXDisplay, xavaXWindow, &xwa);
	if(strcmp(p->winA, "none"))
		XMoveWindow(xavaXDisplay, xavaXWindow, p->wx, p->wy);

	// query for the RR extension in X11
	int error;
	xavaSupportsRR = XRRQueryExtension(xavaXDisplay, &xavaRREventBase, &error);
	if(xavaSupportsRR) {
		int rr_major, rr_minor;

		if(XRRQueryVersion(xavaXDisplay, &rr_major, &rr_minor)) {
			int rr_mask = RRScreenChangeNotifyMask;
			if(rr_major == 1 && rr_minor <= 1) {
				rr_mask &= ~(RRCrtcChangeNotifyMask |
					RROutputChangeNotifyMask |
					RROutputPropertyNotifyMask);
			}

			// listen for display configure events only if enabled
			if(reloadOnDisplayConfigure)
				XRRSelectInput(xavaXDisplay, xavaXWindow, rr_mask);
		}
	}

	return 0;
}

int render_gradient_x(struct config_params *p) {
	if(gradientBox != 0) XFreePixmap(xavaXDisplay, gradientBox);

	gradientBox = XCreatePixmap(xavaXDisplay, xavaXWindow, (unsigned int)p->bw, (unsigned int)p->h, 32);
	// TODO: Error checks

	for(unsigned int I = 0; I < (unsigned int)p->h; I++) {
		// don't touch +1.0/w_h at the end fixes some math problems
		double step = (double)(I%((unsigned int)p->h/(p->gradients-1)))/(double)((unsigned int)p->h/(p->gradients-1));

		// to future devs: this isnt ARGB. this is something internal that cannot be changed by simple means

		unsigned int gcPhase = (p->gradients-1)*I/(unsigned int)p->h;
		xgrad[p->gradients].red   = UNSIGNED_TRANS(xgrad[gcPhase].red, xgrad[gcPhase+1].red, step);
		xgrad[p->gradients].green = UNSIGNED_TRANS(xgrad[gcPhase].green, xgrad[gcPhase+1].green, step);
		xgrad[p->gradients].blue  = UNSIGNED_TRANS(xgrad[gcPhase].blue, xgrad[gcPhase+1].blue, step);
		xgrad[p->gradients].flags = DoRed | DoGreen | DoBlue;
		XAllocColor(xavaXDisplay, xavaXColormap, &xgrad[p->gradients]);

		XSetForeground(xavaXDisplay, xavaXGraphics, xgrad[p->gradients].pixel);
		XFillRectangle(xavaXDisplay, gradientBox, xavaXGraphics, 0, p->h - (int)I, (unsigned int)p->w, 1);
	}
	return 0;
}

EXP_FUNC void xavaOutputClear(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	#if defined(GLX)
		glClearColor(xbgcol.red/65535.0, xbgcol.green/65535.0, xbgcol.blue/65535.0, p->transF ? 1.0*p->background_opacity : 1.0); // TODO BG transparency
		glColors[0] = xcol.red/65535.0;
		glColors[1] = xcol.green/65535.0;
		glColors[2] = xcol.blue/65535.0;
		glColors[3] = p->transF ? p->foreground_opacity : 1.0;

		if(p->shdw) {
			glColors[4] = ARGB_A_32(p->shdw_col);
			glColors[5] = ARGB_R_32(p->shdw_col);
			glColors[6] = ARGB_G_32(p->shdw_col);
			glColors[7] = ARGB_B_32(p->shdw_col);
		}
	#else
		XSetBackground(xavaXDisplay, xavaXGraphics, xbgcol.pixel);
		XClearWindow(xavaXDisplay, xavaXWindow);

		if(p->gradients) render_gradient_x(p);
	#endif
}

EXP_FUNC int xavaOutputApply(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	calculateColors(p);
	// Gets the monitors resolution
	if(p->fullF){
		p->w = DisplayWidth(xavaXDisplay, xavaXScreenNumber);
		p->h = DisplayHeight(xavaXDisplay, xavaXScreenNumber);
	}

	//Atom xa = XInternAtom(xavaXDisplay, "_NET_WM_WINDOW_TYPE", 0); May be used in the future
	//Atom prop;

	// change window type (this makes sure that compoziting managers don't mess with it)
	//if(xa != NULL)
	//{
	//	prop = XInternAtom(xavaXDisplay, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
	//	XChangeProperty(xavaXDisplay, xavaXWindow, xa, XA_ATOM, 32, PropModeReplace, (unsigned char *) &prop, 1);
	//}
	// The code above breaks stuff, please don't use it.	
	

	// tell the window manager to switch to a fullscreen state
	xev.xclient.type=ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = 1;
	xev.xclient.window = xavaXWindow;
	xev.xclient.message_type = wmState;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = p->fullF ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xev.xclient.data.l[1] = fullScreen;
	xev.xclient.data.l[2] = 0;
	XSendEvent(xavaXDisplay, xavaXRoot, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);

	// do the usual stuff :P
	#ifdef GLX
	glViewport(0, 0, p->w, p->h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	
	glOrtho(0, (double)p->w, 0, (double)p->h, -1, 1);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Vsync causes problems on NVIDIA GPUs, looking for possible workarounds/fixes
	glXSwapIntervalEXT(xavaXDisplay, xavaXWindow, p->vsync);
	#endif
	xavaOutputClear(hand);

	if(!p->interactF){
		XRectangle rect;
		XserverRegion region = XFixesCreateRegion(xavaXDisplay, &rect, 1);
		XFixesSetWindowShapeRegion(xavaXDisplay, xavaXWindow, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(xavaXDisplay, region);
	}

	return 0;
}

EXP_FUNC XG_EVENT xavaOutputHandleInput(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// this way we avoid event stacking which requires a full frame to process a single event
	XG_EVENT action = XAVA_IGNORE;

	while(XPending(xavaXDisplay)) {
		XNextEvent(xavaXDisplay, &xavaXEvent);
		switch(xavaXEvent.type) {
			case KeyPress:
			{
				KeySym key_symbol;
				key_symbol = XkbKeycodeToKeysym(xavaXDisplay, xavaXEvent.xkey.keycode, 0, xavaXEvent.xkey.state & ShiftMask ? 1 : 0);
				switch(key_symbol) {
					// should_reload = 1
					// resizeTerminal = 2
					// bail = -1
					case XK_a:
						p->bs++;
						return XAVA_RESIZE;
					case XK_s:
						if(p->bs > 0) p->bs--;
						return XAVA_RESIZE;
					case XK_f: // fullscreen
						p->fullF = !p->fullF;
						return XAVA_RESIZE;
					case XK_Up:
						p->sens *= 1.05;
						break;
					case XK_Down:
						p->sens *= 0.95;
						break;
					case XK_Left:
						p->bw++;
						return XAVA_RESIZE;
					case XK_Right:
						if (p->bw > 1) p->bw--;
						return XAVA_RESIZE;
					case XK_r: //reload config
						return XAVA_RELOAD;
					case XK_q:
						return XAVA_QUIT;
					case XK_Escape:
						return XAVA_QUIT;
					case XK_b:
						xbgcol.red   = rand();
						xbgcol.green = rand();
						xbgcol.blue  = rand();
						xbgcol.flags = DoRed | DoGreen | DoBlue;
						XAllocColor(xavaXDisplay, xavaXColormap, &xbgcol);
						return XAVA_REDRAW;
					case XK_c:
						if(p->gradients) break;
						xcol.red   = rand();
						xcol.green = rand();
						xcol.blue  = rand();
						xcol.flags = DoRed | DoGreen | DoBlue;
						XAllocColor(xavaXDisplay, xavaXColormap, &xcol);
						return XAVA_REDRAW;
				}
				break;
			}
			case ConfigureNotify:
			{
				// the window should not be resized when it IS the monitor
				if(rootWindowEnabled||overrideRedirectEnabled)
					break;

				// This is needed to track the window size
				XConfigureEvent trackedXavaXWindow;
				trackedXavaXWindow = xavaXEvent.xconfigure;
				if(p->w != trackedXavaXWindow.width || p->h != trackedXavaXWindow.height) {
					p->w = trackedXavaXWindow.width;
					p->h = trackedXavaXWindow.height;
				}
				action = XAVA_RESIZE;
				break;
			}
			case Expose:
				if(action != XAVA_RESIZE)
					action = XAVA_REDRAW;
				break;
			case VisibilityNotify:
				if(xavaXEvent.xvisibility.state == VisibilityUnobscured)
					action = XAVA_RESIZE;
				break;
			case ClientMessage:
				if((Atom)xavaXEvent.xclient.data.l[0] == wm_delete_window)
					return XAVA_QUIT;
				break;
			default:
				if(xavaXEvent.type == xavaRREventBase + RRScreenChangeNotify) {
					xavaLog("Display change detected - Restarting...\n");
					return XAVA_RELOAD;
				}
		}
	}
	return action;
}

EXP_FUNC void xavaOutputDraw(struct XAVA_HANDLE *hand) {
	struct config_params *p = &hand->conf;

	// im lazy, but i just wanna make it work
	int xoffset = hand->rest, yoffset = p->h;
	if(rootWindowEnabled) {
		xoffset+=p->wx;
		yoffset+=p->wy;
	}

	#if defined(GLX)
		glClear(GL_COLOR_BUFFER_BIT);

		for(int i=0; i<p->gradients; i++) {
			gradColors[i*3] = xgrad[i].red/65535.0;
			gradColors[i*3+1] = xgrad[i].green/65535.0;
			gradColors[i*3+2] = xgrad[i].blue/65535.0;
		}
		if(drawGLBars(hand, glColors, gradColors)) exit(EXIT_FAILURE);
		
		glXSwapBuffers(xavaXDisplay, xavaXWindow);
		glFinish();
		glXWaitGL();
	#else
		// draw bars on the X11 window
		for(int i = 0; i < hand->bars; i++) {
			// this fixes a rendering bug
			if(hand->f[i] > p->h) hand->f[i] = p->h;

			if(hand->f[i] > hand->fl[i]) {
				if(p->gradients)
					XCopyArea(xavaXDisplay, gradientBox, xavaXWindow, xavaXGraphics, 
						0, p->h - hand->f[i], p->bw, hand->f[i]-hand->fl[i],
						hand->rest + i*(p->bs+p->bw), p->h - hand->f[i]);
				else {
					XSetForeground(xavaXDisplay, xavaXGraphics, xcol.pixel);
					XFillRectangle(xavaXDisplay, xavaXWindow, xavaXGraphics,
						xoffset + i*(p->bs+p->bw), yoffset - hand->f[i], 
						p->bw, hand->f[i]-hand->fl[i]);
				}
			} else if (hand->f[i] < hand->fl[i]) {
				XClearArea(xavaXDisplay, xavaXWindow,
					xoffset + i*(p->bs+p->bw), yoffset - hand->fl[i], 
					p->bw, hand->fl[i]-hand->f[i], 0);
			}
		}
		XSync(xavaXDisplay, 0);
	#endif
	return;
}

EXP_FUNC void xavaOutputCleanup(struct XAVA_HANDLE *hand) {
	// Root mode leaves artifacts on screen even though the window is dead
	XClearWindow(xavaXDisplay, xavaXWindow);

	// make sure that all events are dead by this point
	XSync(xavaXDisplay, 1);

	if(gradientBox != 0) { XFreePixmap(xavaXDisplay, gradientBox); gradientBox = 0; };
 
	#ifdef GLX
		glXMakeCurrent(xavaXDisplay, 0, 0);
		glXDestroyContext(xavaXDisplay, xavaGLXContext);
	#endif
	XFreeGC(xavaXDisplay, xavaXGraphics);
	XDestroyWindow(xavaXDisplay, xavaXWindow);
	XFreeColormap(xavaXDisplay, xavaXColormap);
	XCloseDisplay(xavaXDisplay);
	free(xgrad);
	free(monitorName);
	return;
}

EXP_FUNC void xavaOutputHandleConfiguration(struct XAVA_HANDLE *hand, void *data) {
	struct config_params *p = &hand->conf;

	dictionary *ini = (dictionary*)data;

	reloadOnDisplayConfigure = iniparser_getboolean
		(ini, "x11:reload_on_display_configure", 0);
	rootWindowEnabled = iniparser_getboolean
		(ini, "x11:root_window", 0);
	overrideRedirectEnabled = iniparser_getboolean
		(ini, "x11:override_redirect", 0);
	monitorName = strdup(iniparser_getstring
		(ini, "x11:monitor_name", "none"));

	// who knew that messing with some random properties breaks things
	xavaWarnCondition(rootWindowEnabled&&overrideRedirectEnabled,
			"'root_window' and 'override_redirect' don't work together!");

	xavaBailCondition(rootWindowEnabled&&p->gradients,
			"'root_window' and gradients don't work together!");

	// Xquartz doesnt support ARGB windows
	// Therefore transparency is impossible on macOS
	#ifdef __APPLE__
		p->transF = 0;
	#endif

	#ifndef GLX
		// VSync doesnt work without OpenGL :(
		p->vsync = 0;
	#else
		// OGL + Root Window = completely broken
		rootWindowEnabled = 0;
	#endif
}

