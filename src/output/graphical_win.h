#include <windows.h>
#include <windowsx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#define WGL_WGLEXT_PROTOTYPES
#include <GL/wglext.h>
#include <dwmapi.h>
#include <assert.h>
#include <tchar.h>

#include "graphical.h"

#ifdef assert
#define vertify(expr) if(!expr) assert(0)
#else 
#define vertify(expr) expr
#endif

// define functions
int init_window_win();
void clear_screen_win();
void apply_win_settings();
int get_window_input_win();
void draw_graphical_win(int bars, int rest, int f[200]);
void cleanup_graphical_win(void);
