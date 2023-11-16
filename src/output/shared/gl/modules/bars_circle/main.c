#include <stdio.h>
#include <string.h>
#include <math.h>

#include "output/shared/gl/util/shader.h"
#include "output/shared/gl/util/misc.h"
#include "output/shared/gl/main.h"

#include "shared.h"
#include "output/shared/graphical.h"

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
    struct bars {
        float min_height;
        float start_height;
        float width;
        bool invert;
    } bars;
    struct rotation {
        float per_minute;
        float intensity_scale;
        bool using_intensity;
        bool invert;
    } rotation;
} gl_options;

/**
 * This function is used for handling file change notifications.
 */
EXP_FUNC void xava_gl_module_ionotify_callback(XAVA_IONOTIFY_EVENT event,
                                const char *filename,
                                int id,
                                XAVA* xava) {
    UNUSED(filename);
    UNUSED(id);
    UNUSED(xava);
    switch(event) {
        case XAVA_IONOTIFY_CHANGED:
            shouldRestart = true;
            break;
        default:
            // noop
            break;
    }
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModule *module, XAVA *xava) {
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_VERT, "",   module, xava);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_FRAG, "",   module, xava);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_CONFIG, "", module, xava);

    gl_options.bars.start_height =
        xavaConfigGetF64(pre.config, "bars", "start_height", 0.2);
    gl_options.bars.min_height =
        xavaConfigGetF64(pre.config, "bars", "minimum_height", 0.01);
    gl_options.bars.width =
        xavaConfigGetF64(pre.config, "bars", "width", 0.5);
    gl_options.bars.invert =
        xavaConfigGetBool(pre.config, "bars", "invert", false);

    // handle all user bullshit in one go
    xavaBailCondition(gl_options.bars.start_height >= 1.0 ||
            gl_options.bars.start_height < 0.0,
            "\"start_height\" cannot be under 0.0 or equal/above 1.0");
    xavaBailCondition(gl_options.bars.min_height >= 1.0 ||
            gl_options.bars.min_height < 0.0,
            "\"minimum_height\" cannot be under 0.0 or equal/above 1.0");
    xavaBailCondition(gl_options.bars.start_height + gl_options.bars.min_height >= 1.0,
            "\"minimum_height\" cannot be over or equal to 1.0");
    xavaBailCondition(gl_options.bars.width <= 0.0 || gl_options.bars.width > 1.0,
            "bar \"width\" cannot be wider than 1.0 or smaller than/equal to 0.0");

    gl_options.rotation.per_minute =
        xavaConfigGetF64(pre.config, "rotation", "per_minute", 1.0);
    gl_options.rotation.using_intensity =
        xavaConfigGetBool(pre.config, "rotation", "using_intensity", false);
    gl_options.rotation.invert =
        xavaConfigGetBool(pre.config, "rotation", "invert", false);
    gl_options.rotation.intensity_scale =
        xavaConfigGetF64(pre.config, "rotation", "intensity_scale", 1.0);

    // a little easter egg hehe
    xavaBailCondition(gl_options.rotation.per_minute < 0.0,
            "You cannot time travel, sorry..");

    xavaBailCondition(xava->conf.autobars == true,
            "Since \"bars_circle\" can't rely on window geometry for bars, "
            "you MUST specify the number of bars in [general]. Sorry about that.");
    xava->conf.flag.ignoreWindowSize = true;
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

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
        gradientColor = malloc(4*sizeof(GLfloat)*arr_count(conf->gradients));

    for(uint32_t i=0; i<arr_count(conf->gradients); i++) {
        uint32_t grad_col;
        sscanf(conf->gradients[i], "#%x", &grad_col);
        gradientColor[i*4+0] = ARGB_R_32(grad_col) / 255.0;
        gradientColor[i*4+1] = ARGB_G_32(grad_col) / 255.0;
        gradientColor[i*4+2] = ARGB_B_32(grad_col) / 255.0;
        //gradientColor[i*4+3] = ARGB_A_32(grad_col) / 255.0;
        gradientColor[i*4+3] = conf->foreground_opacity;
    }

    shouldRestart = false;
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

    glUseProgram(pre.program);

    // reallocate and attach verticies data
    REALLOC_SELF(vertexData, xava->bars*12);
    glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    // do image scaling
    projectionMatrix[0] = (float)xava->inner.w/xava->outer.w;
    projectionMatrix[5] = (float)xava->inner.h/xava->outer.h;

    float center_fixed_x = xava->inner.x+xava->inner.w/2.0;
    float center_fixed_y = xava->inner.y+xava->inner.h/2.0;

    // do image translation
    projectionMatrix[3] = (float)center_fixed_x/xava->outer.w*2.0 - 1.0;
    projectionMatrix[7] = 1.0 - (float)center_fixed_y/xava->outer.h*2.0;

    //xavaLog("Scale X/Y: %f %f Translation X/Y: %f %f",
    //        projectionMatrix[0], projectionMatrix[5],
    //        projectionMatrix[3], projectionMatrix[7]);

    glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

    // update screen resoltion
    glUniform2f(PRE_RESOLUTION, xava->outer.w, xava->outer.h);

    // update spacing info
    glUniform1f(PRE_REST,        (float)xava->rest);
    glUniform1f(PRE_BAR_WIDTH,   (float)xava->conf.bw);
    glUniform1f(PRE_BAR_SPACING, (float)xava->conf.bs);
    glUniform1f(PRE_BAR_COUNT,   (float)xava->bars);
    glUniform1f(PRE_AUDIO_RATE,  (float)conf->inputsize);

    u32 gradient_count = arr_count(conf->gradients);
    glUniform1f(PRE_GRAD_SECT_COUNT, gradient_count ? gradient_count-1 : 0);
    glUniform4fv(PRE_GRADIENTS, gradient_count, gradientColor);

    // "clear" the screen
    xava_gl_module_clear(options);
}

EXP_FUNC void xava_gl_module_event(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;

    // check if the visualizer bounds were changed
    if((xava->inner.w != xava->bar_space.w) ||
       (xava->inner.h != xava->bar_space.h)) {
        xava->bar_space.w = xava->inner.w;
        xava->bar_space.h = xava->inner.h;
        pushXAVAEventStack(options->events, XAVA_RESIZE);
        return; // prority
    }

    return;
}

// The original intention of this was to be called when the screen buffer was "unsafe" or "dirty"
// This is not needed in EGL since glClear() is called on each frame. HOWEVER, this clear function
// is often preceded by a slight state change such as a color change, so we pass color info to the
// shaders HERE and ONLY HERE.
EXP_FUNC void xava_gl_module_clear(XAVAGLModuleOptions *options) {
    XAVA *xava   = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

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
}

#define xava_gl_module_draw_rotate_x(x, y, a) (x*cos(a)-y*sin(a))
#define xava_gl_module_draw_rotate_y(x, y, a) (x*sin(a)+y*cos(a))

// global because you're annoying
float additional_angle = 0.0;

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    XAVA   *xava  = options->xava;

    float intensity = xava_gl_module_util_calculate_intensity(xava);
    float time      = xava_gl_module_util_obtain_time();

    /**
     * Here we start rendering to the texture
     **/

    const float min_height   = gl_options.bars.min_height;
    const float start_height = gl_options.bars.start_height;
    const float bar_width    = gl_options.bars.width;

    if(gl_options.rotation.using_intensity == false) {
        additional_angle = fmod(2.0*M_PI*xava_gl_module_util_obtain_time()/60.0*
            gl_options.rotation.per_minute, 2.0*M_PI);
        if(gl_options.rotation.invert)
            additional_angle *= -1.0;
    } else {
        additional_angle += xava_gl_module_util_calculate_intensity(xava) *
            0.01 * (gl_options.rotation.invert ? -1.0 : 1.0) *
            gl_options.rotation.intensity_scale;
        additional_angle = fmod(additional_angle, 2*M_PI);
    }

    for(register uint32_t i=0; i<xava->bars; i++) {
        float angle = i*2.0*M_PI/xava->bars;
        float x1 = -1.0/xava->bars*bar_width;
        float y1 = start_height;
        float x2 = -x1;
        float y2 = y1;

        if(gl_options.bars.invert)
            angle *= -1.0;

        angle += additional_angle;

        float height = (float)xava->f[i]/xava->inner.h*(1.0-y1);
        y2 += height > min_height ? height : min_height;

        // top left
        vertexData[i*12]    = xava_gl_module_draw_rotate_x(x1, y2, angle);
        vertexData[i*12+1]  = xava_gl_module_draw_rotate_y(x1, y2, angle);
        // bottom left
        vertexData[i*12+2]  = xava_gl_module_draw_rotate_x(x1, y1, angle);
        vertexData[i*12+3]  = xava_gl_module_draw_rotate_y(x1, y1, angle);
        // bottom right
        vertexData[i*12+4]  = xava_gl_module_draw_rotate_x(x2, y1, angle);
        vertexData[i*12+5]  = xava_gl_module_draw_rotate_y(x2, y1, angle);
        // top right
        vertexData[i*12+6]  = xava_gl_module_draw_rotate_x(x2, y2, angle);
        vertexData[i*12+7]  = xava_gl_module_draw_rotate_y(x2, y2, angle);
        // bottom right
        vertexData[i*12+8]  = xava_gl_module_draw_rotate_x(x2, y1, angle);
        vertexData[i*12+9]  = xava_gl_module_draw_rotate_y(x2, y1, angle);
        // top left
        vertexData[i*12+10] = xava_gl_module_draw_rotate_x(x1, y2, angle);
        vertexData[i*12+11] = xava_gl_module_draw_rotate_y(x1, y2, angle);
    }

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME,      time);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    glEnableVertexAttribArray(PRE_BARS);

    // You use the number of verticies, but not the actual polygon count
    glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

    glDisableVertexAttribArray(PRE_BARS);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    UNUSED(options);

    // delete both pipelines
    xava_gl_module_program_destroy(&pre);

    free(gradientColor);
    free(vertexData);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

