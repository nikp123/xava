#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmd.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/XKBlib.h>
#include <X11/extensions/shape.h>
#include <X11/extensions/Xfixes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "graphical.h"

Pixmap gradientBox = 0;
XColor xbgcol, xcol, *xgrad;
int gradcount;

XEvent gavaXEvent;
Colormap gavaXColormap;
Display *gavaXDisplay;
Screen *gavaXScreen;
Window gavaXWindow, gavaXRoot;
GC gavaXGraphics;

XVisualInfo gavaVInfo;
XSetWindowAttributes gavaAttr;
Atom wm_delete_window, wmState, fullScreen, mwmHintsProperty, wmStateBelow;
XClassHint gavaXClassHint;
XWMHints gavaXWMHints;
XEvent xev;


// mwmHints helps us comunicate with the window manager
struct mwmHints {
    unsigned long flags;
    unsigned long functions;
    unsigned long decorations;
    long input_mode;
    unsigned long status;
};

int gavaXScreenNumber, shadow, shadow_color, GLXmode;
static unsigned int defaultColors[8] = {0x00000000,0x00FF0000,0x0000FF00,0x00FFFF00,0x000000FF,0x00FF00FF,0x0000FFFF,0x00FFFFFF};


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
int XGLInit() {
	// we will use the existing VisualInfo for this, because I'm not messing around with FBConfigs
	gavaGLXContext = glXCreateContext(gavaXDisplay, &gavaVInfo, NULL, 1);
	glXMakeCurrent(gavaXDisplay, gavaXWindow, gavaGLXContext);
	if(transparentFlag) {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	return 0;
}
#endif

void calculateColors(char *color, char *bcolor, int bgcol, int col) {
	char tempColorStr[8];
	
	// Generate a sum of colors
	if(!strcmp(color, "default")) {
		unsigned long redSum = 0, greenSum = 0, blueSum = 0;
		XColor tempColor;
		int xPrecision = 20, yPrecision = 20;	
		while(1){ if(((double)gavaXScreen->width / (double)xPrecision) == ((int)gavaXScreen->width / (int)xPrecision)) break; else xPrecision++; }
		while(1){ if(((double)gavaXScreen->height / (double)yPrecision) == ((int)gavaXScreen->height / (int)yPrecision)) break; else yPrecision++; }
		
		// we need a source for that
		XImage *background = XGetImage(gavaXDisplay, gavaXRoot, 0, 0, gavaXScreen->width, gavaXScreen->height, AllPlanes, XYPixmap);
		for(unsigned short i = 0; i < gavaXScreen->width; i+=(gavaXScreen->width / xPrecision)) {
			for(unsigned short I = 0; I < gavaXScreen->height; I+=(gavaXScreen->height / yPrecision)) {
				// we validate each and EVERY pixel value,
				// because Xorg says so..... and is really slow, so we make compromises
				tempColor.pixel = XGetPixel(background, i, I);
				XQueryColor(gavaXDisplay, gavaXColormap, &tempColor);	
				
				redSum += tempColor.red/256;
				greenSum += tempColor.green/256;
				blueSum += tempColor.blue/256;
			}
		}
		redSum /= xPrecision*yPrecision;
		greenSum /= xPrecision*yPrecision;
		blueSum /= xPrecision*yPrecision;

		XDestroyImage(background);
		sprintf(tempColorStr, "#%02hhx%02hhx%02hhx", (unsigned char)(redSum), (unsigned char)(greenSum), (unsigned char)(blueSum));
	} else if(color[0] != '#') 
		sprintf(tempColorStr, "#%02hhx%02hhx%02hhx", (unsigned char)((defaultColors[col]>>16)%256), (unsigned char)((defaultColors[col]>>8)%256), (unsigned char)(defaultColors[col]));
	
	XParseColor(gavaXDisplay, gavaXColormap, color[0]=='#' ? color : tempColorStr, &xcol);
	XAllocColor(gavaXDisplay, gavaXColormap, &xcol);

	
	if(bcolor[0] != '#')
		sprintf(tempColorStr, "#%02hhx%02hhx%02hhx", (unsigned char)(defaultColors[bgcol]>>16), (unsigned char)(defaultColors[bgcol]>>8), (unsigned char)(defaultColors[bgcol]));
	
	XParseColor(gavaXDisplay, gavaXColormap, bcolor[0]=='#' ? bcolor : tempColorStr, &xbgcol);
	XAllocColor(gavaXDisplay, gavaXColormap, &xbgcol);
}

int init_window_x(char *color, char *bcolor, int col, int bgcol, int set_win_props, char **argv, int argc, int gradient, char **gradient_colors, int gradient_count, unsigned int shdw, unsigned int shdw_col, int w, int h)
{
	// Pass the shadow values
	shadow = shdw;
	shadow_color = shdw_col;
	
	// connect to the X server
	gavaXDisplay = XOpenDisplay(NULL);
	if(gavaXDisplay == NULL) {
		fprintf(stderr, "cannot open X display\n");
		return 1;
	}
	
	gavaXScreen = DefaultScreenOfDisplay(gavaXDisplay);
	gavaXScreenNumber = DefaultScreen(gavaXDisplay);
	gavaXRoot = RootWindow(gavaXDisplay, gavaXScreenNumber);
	
	calculate_win_pos(&windowX, &windowY, w, h, gavaXScreen->width, gavaXScreen->height, windowAlignment);
	
	// 32 bit color means alpha channel support
	XMatchVisualInfo(gavaXDisplay, gavaXScreenNumber, transparentFlag ? 32 : 24, TrueColor, &gavaVInfo);
		gavaAttr.colormap = XCreateColormap(gavaXDisplay, DefaultRootWindow(gavaXDisplay), gavaVInfo.visual, AllocNone);
		gavaXColormap = gavaAttr.colormap; 
		calculateColors(color, bcolor, bgcol, col);
		gavaAttr.background_pixel = transparentFlag ? 0 : xbgcol.pixel;
		gavaAttr.border_pixel = xcol.pixel;
	
	gavaXWindow = XCreateWindow(gavaXDisplay, gavaXRoot, windowX, windowY, w, h, 0, gavaVInfo.depth, InputOutput, gavaVInfo.visual, CWEventMask | CWColormap | CWBorderPixel | CWBackPixel, &gavaAttr);	
	XStoreName(gavaXDisplay, gavaXWindow, "GAVA");

	// The "X" button is handled by the window manager and not Xorg, so we set up a Atom
	wm_delete_window = XInternAtom (gavaXDisplay, "WM_DELETE_WINDOW", 0);
	XSetWMProtocols(gavaXDisplay, gavaXWindow, &wm_delete_window, 1);
	
	if(set_win_props) {
		gavaXWMHints.flags = InputHint | StateHint;
		gavaXWMHints.initial_state = NormalState;
		gavaXClassHint.res_name = (char *)"Gava";
		XmbSetWMProperties(gavaXDisplay, gavaXWindow, NULL, NULL, argv, argc, NULL, &gavaXWMHints, &gavaXClassHint);
	}

	XSelectInput(gavaXDisplay, gavaXWindow, VisibilityChangeMask | StructureNotifyMask | ExposureMask | KeyPressMask | KeymapNotify);
	
	#ifdef GLX
		if(GLXmode) if(XGLInit()) return 1;
	#endif
	
	XMapWindow(gavaXDisplay, gavaXWindow);
	gavaXGraphics = XCreateGC(gavaXDisplay, gavaXWindow, 0, 0);
	
	if(gradient) {
		xgrad = malloc((gradient_count+1)*sizeof(XColor));
		XParseColor(gavaXDisplay, gavaXColormap, gradient_colors[0], &xgrad[gradient_count]);
		XAllocColor(gavaXDisplay, gavaXColormap, &xgrad[gradient_count]);
		for(int i=0; i<gradient_count; i++) {
			XParseColor(gavaXDisplay, gavaXColormap, gradient_colors[i], &xgrad[i]);
			XAllocColor(gavaXDisplay, gavaXColormap, &xgrad[i]);
		}
		gradcount=gradient_count;
	}
	
	// Set up atoms
	wmState = XInternAtom(gavaXDisplay, "_NET_WM_STATE", 0);
	fullScreen = XInternAtom(gavaXDisplay, "_NET_WM_STATE_FULLSCREEN", 0);
	mwmHintsProperty = XInternAtom(gavaXDisplay, "_MOTIF_WM_HINTS", 0);
	wmStateBelow = XInternAtom(gavaXDisplay, "_NET_WM_STATE_BELOW", 1);

	if(keepInBottom){
	/**
		window  = the respective client window
		message_type = _NET_WM_STATE
		format = 32
		data.l[0] = the action, as listed below
		data.l[1] = first property to alter
		data.l[2] = second property to alter
		data.l[3] = source indication (0-unk,1-normal app,2-pager)
		other data.l[] elements = 0
	**/
		xev.xclient.type = ClientMessage;
		xev.xclient.window = gavaXWindow;
		xev.xclient.message_type = wmState;
		xev.xclient.format = 32;
		xev.xclient.data.l[0] = _NET_WM_STATE_ADD;
		xev.xclient.data.l[1] = wmStateBelow; // Keeps the window below duh
		xev.xclient.data.l[2] = 0;
		xev.xclient.data.l[3] = 0;
		xev.xclient.data.l[4] = 0;
		XSendEvent(gavaXDisplay, gavaXRoot, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	}

	// Setting window options			
	struct mwmHints hints;
	hints.flags = (1L << 1);
	hints.decorations = borderFlag;
	XChangeProperty(gavaXDisplay, gavaXWindow, mwmHintsProperty, mwmHintsProperty, 32, PropModeReplace, (unsigned char *)&hints, 5);
	
	// move the window in case it didn't by default
	XWindowAttributes xwa;
	XGetWindowAttributes(gavaXDisplay, gavaXWindow, &xwa);
	if(strcmp(windowAlignment, "none"))
		XMoveWindow(gavaXDisplay, gavaXWindow, windowX, windowY);
	
	return 0;
}

void clear_screen_x(void) {
	if(GLXmode) return;	
	XSetBackground(gavaXDisplay, gavaXGraphics, xbgcol.pixel);
	XClearWindow(gavaXDisplay, gavaXWindow);
}

int apply_window_settings_x(int *w, int *h)
{
	// Gets the monitors resolution
	if(fs){
		(*w) = DisplayWidth(gavaXDisplay, gavaXScreenNumber);
		(*h) = DisplayHeight(gavaXDisplay, gavaXScreenNumber);
	}

	//Atom xa = XInternAtom(gavaXDisplay, "_NET_WM_WINDOW_TYPE", 0); May be used in the future
	//Atom prop;

	// change window type (this makes sure that compoziting managers don't mess with it)
	//if(xa != NULL)
	//{
	//	prop = XInternAtom(gavaXDisplay, "_NET_WM_WINDOW_TYPE_DESKTOP", 0);
	//	XChangeProperty(gavaXDisplay, gavaXWindow, xa, XA_ATOM, 32, PropModeReplace, (unsigned char *) &prop, 1);
	//}
	// The code above breaks stuff, please don't use it.	
	

	// tell the window manager to switch to a fullscreen state
	xev.xclient.type=ClientMessage;
	xev.xclient.serial = 0;
	xev.xclient.send_event = 1;
	xev.xclient.window = gavaXWindow;
	xev.xclient.message_type = wmState;
	xev.xclient.format = 32;
	xev.xclient.data.l[0] = fs ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xev.xclient.data.l[1] = fullScreen;
	xev.xclient.data.l[2] = 0;
	XSendEvent(gavaXDisplay, gavaXRoot, 0, SubstructureRedirectMask | SubstructureNotifyMask, &xev);
	
	// do the usual stuff :P
	if(GLXmode){	
		#ifdef GLX
		glViewport(0, 0, (double)*w, (double)*h);
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		
		glOrtho(0, (double)*w, 0, (double)*h, -1, 1);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		#endif
	} else clear_screen_x();

	if(!interactable){	
		XRectangle rect;
		XserverRegion region = XFixesCreateRegion(gavaXDisplay, &rect, 1);
		XFixesSetWindowShapeRegion(gavaXDisplay, gavaXWindow, ShapeInput, 0, 0, region);
		XFixesDestroyRegion(gavaXDisplay, region);
	}

	return 0;
}

int get_window_input_x(int *should_reload, int *bs, double *sens, int *bw, int *w, int *h, char *color, char *bcolor, int gradient) {
	while(!*should_reload && XPending(gavaXDisplay)) {
		XNextEvent(gavaXDisplay, &gavaXEvent);
		
		switch(gavaXEvent.type) {
			case KeyPress:
			{
				KeySym key_symbol;
				key_symbol = XkbKeycodeToKeysym(gavaXDisplay, gavaXEvent.xkey.keycode, 0, gavaXEvent.xkey.state & ShiftMask ? 1 : 0);
				switch(key_symbol) {
					// should_reload = 1
					// resizeTerminal = 2
					// bail = -1
					case XK_a:
						(*bs)++;
						return 2;
					case XK_s:
						if((*bs) > 0) (*bs)--;
						return 2;
					case XK_f: // fullscreen
						fs = !fs;
						return 2;
					case XK_Up:
						(*sens) *= 1.05;
						break;
					case XK_Down:
						(*sens) *= 0.95;
						break;
					case XK_Left:
						(*bw)++;
						return 2;
					case XK_Right:
						if ((*bw) > 1) (*bw)--;
						return 2;
					case XK_r: //reload config
						(*should_reload) = 1;
						return 1;
					case XK_q:
						return -1;
					case XK_Escape:
						return -1;
					case XK_b:
						if(transparentFlag) break;
						if(bcolor[0] == '#' && strlen(bcolor) == 7) free(bcolor);
						bcolor = (char *) malloc(8*sizeof(char));
						sprintf(bcolor, "#%hhx%hhx%hhx", (unsigned char)(rand() % 0x100), (unsigned char)(rand() % 0x100), (unsigned char)(rand() % 0x100));
						XParseColor(gavaXDisplay, gavaXColormap, bcolor, &xbgcol);
						XAllocColor(gavaXDisplay, gavaXColormap, &xbgcol);
						return 3;
					case XK_c:
						if(gradient) break;
						if(color[0] == '#' && strlen(color) == 7) free(color);
						color = (char *) malloc(8*sizeof(char));
						sprintf(color, "#%hhx%hhx%hhx", (unsigned char)(rand() % 0x100), (unsigned char)(rand() % 0x100), (unsigned char)(rand() % 0x100));
						XParseColor(gavaXDisplay, gavaXColormap, color, &xcol);
						XAllocColor(gavaXDisplay, gavaXColormap, &xcol);
						return 3;
				}
				break;
			}
			case ConfigureNotify:
			{
				// This is needed to track the window size
				XConfigureEvent trackedGavaXWindow;
				trackedGavaXWindow = gavaXEvent.xconfigure;
				if((*w) != trackedGavaXWindow.width || (*h) != trackedGavaXWindow.height)
				{
					(*w) = trackedGavaXWindow.width;
					(*h) = trackedGavaXWindow.height;
					return 2;
				}
				break;
			}
			case Expose:
				return 3;
			case VisibilityNotify:
				if(gavaXEvent.xvisibility.state == VisibilityUnobscured) return 2;
				break;
			case ClientMessage:
				if((Atom)gavaXEvent.xclient.data.l[0] == wm_delete_window)
					return -1;
				break;
		}
	}
	return 0;
}

int render_gradient_x(int window_height, int bar_width, double foreground_opacity) {
	if(gradientBox != 0) XFreePixmap(gavaXDisplay, gradientBox);

	gradientBox = XCreatePixmap(gavaXDisplay, gavaXWindow, bar_width, window_height, 32);
	// TODO: Error checks

	for(int I = 0; I < window_height; I++) {
		// don't touch +1.0/w_h at the end fixes some math problems
		double step = (double)(I%(window_height/(gradcount-1)))/(double)(window_height/(gradcount-1))+2.0/window_height;

		// gradients break compatibility with non ARGB displays.
		// if you could fix this without allocating bilions of colors, please do so

		int gcPhase = (gradcount-1)*I/window_height;
		xgrad[gradcount].pixel ^= xgrad[gradcount].pixel; 	
		if(xgrad[gcPhase].red != 0 || xgrad[gcPhase+1].red != 0) {
			if(xgrad[gcPhase].red < xgrad[gcPhase+1].red) 
				xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].red + ((xgrad[gcPhase+1].red - xgrad[gcPhase].red) * step)) / 256 << 16;
			else xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].red - ((xgrad[gcPhase].red - xgrad[gcPhase+1].red) * step)) / 256 << 16;
		}
		
		if(xgrad[gcPhase].green != 0 || xgrad[gcPhase+1].green != 0) {
			if(xgrad[gcPhase].green < xgrad[gcPhase+1].green) 
				xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].green + ((xgrad[gcPhase+1].green - xgrad[gcPhase].green) * step)) / 256 << 8;
			else xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].green - ((xgrad[gcPhase].green - xgrad[gcPhase+1].green) * step)) / 256 << 8;
		}
		
		if(xgrad[gcPhase].blue != 0 || xgrad[gcPhase+1].blue != 0) {
			if(xgrad[gcPhase].blue < xgrad[gcPhase+1].blue) 
				xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].blue + ((xgrad[gcPhase+1].blue - xgrad[gcPhase].blue) * step)) / 256;
			else xgrad[gradcount].pixel |= (unsigned long)(xgrad[gcPhase].blue - ((xgrad[gcPhase].blue - xgrad[gcPhase+1].blue) * step)) / 256;
		}
		
		xgrad[gradcount].pixel |= (unsigned int)((unsigned char)(0xFF * foreground_opacity) << 24);	// set window opacity

		XSetForeground(gavaXDisplay, gavaXGraphics, xgrad[gradcount].pixel);
		XFillRectangle(gavaXDisplay, gradientBox, gavaXGraphics, 0, window_height - I, bar_width, 1);
	}
	return 0;
}

void draw_graphical_x(int window_height, int bars_count, int bar_width, int bar_spacing, int rest, int gradient, int f[200], int flastd[200], double foreground_opacity)
{
	if(GLXmode) {
		#ifdef GLX
		glClearColor(xbgcol.red/65535.0, xbgcol.green/65535.0, xbgcol.blue/65535.0, (!transparentFlag)*1.0); // TODO BG transparency
		glClear(GL_COLOR_BUFFER_BIT);
		#endif
	} else {
		if(gradient&&!gradientBox&&transparentFlag) render_gradient_x(window_height, bar_width, foreground_opacity); 
	}
	
	if(GLXmode) {
		#ifdef GLX
		float glColors[8] = {xcol.red/65535.0, xcol.green/65535.0, xcol.blue/65535.0, foreground_opacity, ((unsigned int)shadow_color>>24)%256/255.0,
			 ((unsigned int)shadow_color>>16)%256/255.0, ((unsigned int)shadow_color>>8)%256/255.0, (unsigned int)shadow_color%256/255.0};
		float gradColors[24] = {0.0};
		for(int i=0; i<gradcount; i++) {
			gradColors[i*3] = xgrad[i].red/65535.0;
			gradColors[i*3+1] = xgrad[i].green/65535.0;
			gradColors[i*3+2] = xgrad[i].blue/65535.0;
		}
		if(drawGLBars(rest, bar_width, bar_spacing, bars_count, window_height, shadow, gradient?gradcount:0, glColors, gradColors, f)) exit(EXIT_FAILURE);
		#endif
	} else {	
		// draw bars on the X11 window
		for(int i = 0; i < bars_count; i++) {
			// this fixes a rendering bug
			if(f[i] > window_height) f[i] = window_height;
				
			if(f[i] > flastd[i]) {
				if(gradient)
					XCopyArea(gavaXDisplay, gradientBox, gavaXWindow, gavaXGraphics, 0, window_height - f[i], bar_width, f[i] - flastd[i], rest + i*(bar_spacing+bar_width), window_height - f[i]);	
				else {
					XSetForeground(gavaXDisplay, gavaXGraphics, xcol.pixel);
					XFillRectangle(gavaXDisplay, gavaXWindow, gavaXGraphics, rest + i*(bar_spacing+bar_width), window_height - f[i], bar_width, f[i] - flastd[i]);
				}
			}
			else if (f[i] < flastd[i])
				XClearArea(gavaXDisplay, gavaXWindow, rest + i*(bar_spacing+bar_width), window_height - flastd[i], bar_width, flastd[i] - f[i], 0);
		}
	}
	
	#ifdef GLX
	if(GLXmode) {
		glXSwapBuffers(gavaXDisplay, gavaXWindow);
		glXWaitGL();
	}
	#endif
	
	XSync(gavaXDisplay, 0);
	return;
}

void adjust_x(void){
	if(gradientBox != 0) { XFreePixmap(gavaXDisplay, gradientBox); gradientBox = 0; };
}

void cleanup_graphical_x(void)
{
	// make sure that all events are dead by this point
	XSync(gavaXDisplay, 0);
	
	adjust_x();	
	 
	#ifdef GLX
		glXMakeCurrent(gavaXDisplay, 0, 0);
		if(GLXmode) glXDestroyContext(gavaXDisplay, gavaGLXContext);
	#endif
	XFreeGC(gavaXDisplay, gavaXGraphics);
	XDestroyWindow(gavaXDisplay, gavaXWindow);
	XFreeColormap(gavaXDisplay, gavaXColormap);
	XCloseDisplay(gavaXDisplay);
	free(xgrad);
	return;
}
