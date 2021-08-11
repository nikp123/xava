#ifdef assert
#define vertify(expr) if(!expr) assert(0)
#else
#define vertify(expr) expr
#endif

#define WGL_WGLEXT_PROTOTYPES

