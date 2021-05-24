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
	GLuint texture;
};

void    EGLShadersLoad(struct EGLprogram *pre, struct EGLprogram *post);
EGLint  EGLShaderBuild(const char *source, GLenum shader_type);
void    EGLCreateProgram(struct EGLprogram *program);
