#define WIN32_DISPLAY_NUM 3
#define WIN32_DISPLAY_NAME "win"

#include <windows.h>
#include <windowsx.h>
#include <GL/glew.h>
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
int init_window_win(void);
void clear_screen_win(void);
int apply_win_settings(void);
int get_window_input_win(void);
void draw_graphical_win(int bars, int rest, int f[200], int flastd[200]);
void cleanup_graphical_win(void);
