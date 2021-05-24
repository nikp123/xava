#include <EGL/eglplatform.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>

// this is so that loading shaders is managable
struct EGLprogram {
	char *fragText, *vertText;
	GLuint vert, frag;
	GLuint program;
};

struct FBO {
	GLuint framebuffer;
	GLuint final_texture;
	GLuint depth_texture;
};

struct _escontext {
	// because windowing systems are complicated
	EGLNativeDisplayType native_display;
	EGLNativeWindowType native_window;

	// EGL display
	EGLDisplay  display;
	// EGL context
	EGLContext  context;
	// EGL surface
	EGLSurface  surface;
};

void       EGLShadersLoad(void);
EGLint     EGLShaderBuild(const char *source, GLenum shader_type);
void       EGLCreateProgram(struct EGLprogram *program);
void       EGLInit(struct XAVA_HANDLE *xava);
void       EGLApply(struct XAVA_HANDLE *xava);
void       EGLDraw(struct XAVA_HANDLE *xava);
EGLBoolean EGLCreateContext(struct XAVA_HANDLE *xava, struct _escontext *ESContext);
