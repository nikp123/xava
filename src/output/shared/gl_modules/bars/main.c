#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../shared/shader.h"
#include "../shared/post.h"
#include "../shared/util.h"
#include "../../gl_shared.h"

#include "../../../../shared.h"
#include "../../../graphical.h"

// functions needed by something else
void xava_gl_module_clear(XAVAGLModuleOptions *options);

// we don't really need this struct, but it's nice to have (for extensibility)
xava_gl_module_program pre;

// shader buffers
static GLfloat *vertexData;
static GLfloat *gradientColor;

// used to adjust the view
static GLfloat projectionMatrix[16] =
    {2.0, 0.0,  0.0, -1.0,
     0.0,  2.0,  0.0, -1.0,
     0.0,  0.0, -1.0, -1.0,
     0.0,  0.0,  0.0,  1.0};

// geometry information
static GLuint PRE_REST;
static GLuint PRE_BAR_WIDTH;
static GLuint PRE_BAR_SPACING;
static GLuint PRE_BAR_COUNT;
static GLuint PRE_BARS;
static GLuint PRE_AUDIO;
static GLuint PRE_AUDIO_RATE;
static GLuint PRE_RESOLUTION;
static GLuint PRE_PROJMATRIX;

// color information
static GLuint PRE_FGCOL;
static GLuint PRE_BGCOL;

// system information providers
static GLuint PRE_GRAD_SECT_COUNT;
static GLuint PRE_GRADIENTS;
static GLuint PRE_TIME;
static GLuint PRE_INTENSITY;

// this is hacked in, shut up
static bool shouldRestart;

// renderer hacks and options
struct gl_renderer_options {
    struct color {
        bool blending;
    } color;
} gl_options;

gl_module_post_render post;

/**
 * This function is used for handling file change notifications.
 */
EXP_FUNC void xava_gl_module_ionotify_callback(XAVA_IONOTIFY_EVENT event,
                                const char *filename,
                                int id,
                                struct XAVA_HANDLE* xava) {
    switch(event) {
        case XAVA_IONOTIFY_CHANGED:
            shouldRestart = true;
            break;
        default:
            // noop
            break;
    }
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModuleOptions *options) {
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_VERT, "", options);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_FRAG, "", options);

    post.options = options;
    xava_gl_module_post_config_load(&post);
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava = options->xava;
    struct config_params *conf = &xava->conf;

    // automatically assign this so it isn't invalid during framebuffer creation
    xava->w = conf->w;
    xava->h = conf->h;

    // create programs
    xava_gl_module_program_create(&pre);

    // color
    PRE_FGCOL      = glGetUniformLocation(pre.program, "foreground_color");
    PRE_BGCOL      = glGetUniformLocation(pre.program, "background_color");

    PRE_GRAD_SECT_COUNT = glGetUniformLocation(pre.program, "gradient_sections");
    PRE_GRADIENTS       = glGetUniformLocation(pre.program, "gradient_color");

    // sys info provider
    PRE_TIME       = glGetUniformLocation(pre.program, "time");
    PRE_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

    // geometry
    PRE_BARS          = glGetAttribLocation( pre.program, "fft_bars");
    PRE_AUDIO         = glGetAttribLocation( pre.program, "audio_data");
    PRE_AUDIO_RATE    = glGetUniformLocation(pre.program, "audio_rate");
    PRE_RESOLUTION    = glGetUniformLocation(pre.program, "resolution");
    PRE_REST          = glGetUniformLocation(pre.program, "rest");
    PRE_BAR_WIDTH     = glGetUniformLocation(pre.program, "bar_width");
    PRE_BAR_SPACING   = glGetUniformLocation(pre.program, "bar_spacing");
    PRE_BAR_COUNT     = glGetUniformLocation(pre.program, "bar_count");
    PRE_PROJMATRIX    = glGetUniformLocation(pre.program, "projection_matrix");

    glUseProgram(pre.program);

    glEnable(GL_DEPTH_TEST);

    // we just need working pointers so that realloc() works
    vertexData = malloc(1);

    // gradients
    if(conf->gradients)
        gradientColor = malloc(4*sizeof(GLfloat)*conf->gradients);

    for(int i=0; i<conf->gradients; i++) {
        uint32_t grad_col;
        sscanf(conf->gradient_colors[i], "#%x", &grad_col);
        gradientColor[i*4+0] = ARGB_R_32(grad_col) / 255.0;
        gradientColor[i*4+1] = ARGB_G_32(grad_col) / 255.0;
        gradientColor[i*4+2] = ARGB_B_32(grad_col) / 255.0;
        //gradientColor[i*4+3] = ARGB_A_32(grad_col) / 255.0;
        gradientColor[i*4+3] = conf->foreground_opacity;
    }

    xava_gl_module_post_init(&post);

    shouldRestart = false;
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava = options->xava;
    struct config_params *conf = &xava->conf;

    glUseProgram(pre.program);

    // reallocate and attach verticies data
    vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*12);
    glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    // since most of this information remains untouched, let's precalculate
    for(int i=0; i<xava->bars; i++) {
        vertexData[i*12]    = xava->rest + i*(conf->bs+conf->bw);
        vertexData[i*12+1]  = 1.0f;
        vertexData[i*12+2]  = vertexData[i*12];
        vertexData[i*12+3]  = 0.0f;
        vertexData[i*12+4]  = vertexData[i*12]+conf->bw;
        vertexData[i*12+5]  = 0.0f;
        vertexData[i*12+6]  = vertexData[i*12+4];
        vertexData[i*12+7]  = 1.0f;
        vertexData[i*12+8]  = vertexData[i*12+4];
        vertexData[i*12+9]  = 0.0f;
        vertexData[i*12+10] = vertexData[i*12];
        vertexData[i*12+11] = 1.0f;
    }

    // do image scaling
    projectionMatrix[0] = 2.0/xava->w;
    projectionMatrix[5] = 2.0/xava->h;

    // do image translation
    projectionMatrix[3] = (float)xava->x/xava->w*2.0 - 1.0;
    projectionMatrix[7] = 1.0 - (float)(xava->y+conf->h)/xava->h*2.0;

    glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

    // update screen resoltion
    glUniform2f(PRE_RESOLUTION, xava->w, xava->h);

    // update spacing info
    glUniform1f(PRE_REST,        (float)xava->rest);
    glUniform1f(PRE_BAR_WIDTH,   (float)xava->conf.bw);
    glUniform1f(PRE_BAR_SPACING, (float)xava->conf.bs);
    glUniform1f(PRE_BAR_COUNT,   (float)xava->bars);
    glUniform1f(PRE_AUDIO_RATE,  (float)conf->inputsize);

    glUniform1f(PRE_GRAD_SECT_COUNT, conf->gradients ? conf->gradients-1 : 0);
    glUniform4fv(PRE_GRADIENTS, conf->gradients, gradientColor);

    // "clear" the screen
    xava_gl_module_clear(options);

    xava_gl_module_post_apply(&post);
}

EXP_FUNC XG_EVENT xava_gl_module_event(XAVAGLModuleOptions *options) {
    if(shouldRestart) {
        shouldRestart = false;
        return XAVA_RELOAD;
    }
    return XAVA_IGNORE;
}

// The original intention of this was to be called when the screen buffer was "unsafe" or "dirty"
// This is not needed in EGL since glClear() is called on each frame. HOWEVER, this clear function
// is often preceded by a slight state change such as a color change, so we pass color info to the
// shaders HERE and ONLY HERE.
EXP_FUNC void xava_gl_module_clear(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava   = options->xava;
    struct config_params *conf = &xava->conf;

    // if you want to fiddle with certain uniforms from a shader, YOU MUST SWITCH TO IT
    // (https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes#glUniform_doesn.27t_work)
    glUseProgram(pre.program);

    float fgcol[4] = {
        ARGB_R_32(conf->col)/255.0,
        ARGB_G_32(conf->col)/255.0,
        ARGB_B_32(conf->col)/255.0,
        conf->foreground_opacity
    };

    float bgcol[4] = {
        ARGB_R_32(conf->bgcol)/255.0,
        ARGB_G_32(conf->bgcol)/255.0,
        ARGB_B_32(conf->bgcol)/255.0,
        conf->background_opacity
    };

    glUniform4f(PRE_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);
    glUniform4f(PRE_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);

    //glClearColor(bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE   *xava  = options->xava;

    float intensity = xava_gl_module_util_calculate_intensity(xava);
    float time      = xava_gl_module_util_obtain_time();

    /**
     * Here we start rendering to the texture
     **/

    register GLfloat *d = vertexData;
    for(register int i=0; i<xava->bars; i++) {
        // the speed part
        *(++d)  = xava->f[i];
        *(d+=6) = xava->f[i];
        *(d+=4) = xava->f[i];
        d++;
    }

    // enable blending temporary so that the colors get properly calculated on
    // the shader end of the pre stage
    if(gl_options.color.blending) {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
    }

    // bind render target to texture
    xava_gl_module_post_pre_draw_setup(&post);

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME,      time);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    glEnableVertexAttribArray(PRE_BARS);

    // You use the number of verticies, but not the actual polygon count
    glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

    glDisableVertexAttribArray(PRE_BARS);

    // disable blending on the post stage as it produces
    // invalid colors on the window manager end
    glDisable(GL_BLEND);

    // get out the draw function if post shaders aren't enabled
    xava_gl_module_post_draw(&post);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    // delete both pipelines
    xava_gl_module_program_destroy(&pre);
    xava_gl_module_post_cleanup(&post);

    free(gradientColor);
    free(vertexData);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

