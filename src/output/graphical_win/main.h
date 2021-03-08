// exported function, a macro used to determine which functions
// are exposed as symbols within the final library/obj files
#define EXP_FUNC __attribute__ ((visibility ("default")))

#ifdef assert
#define vertify(expr) if(!expr) assert(0)
#else 
#define vertify(expr) expr
#endif

#define WGL_WGLEXT_PROTOTYPES
#define WIN_ICON_PATH "xava.ico"

