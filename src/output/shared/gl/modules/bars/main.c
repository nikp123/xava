#include <stdio.h>
#include <string.h>
#include <math.h>

#include "output/shared/gl/util/shader.h"
//#include "output/shared/gl/modules/shared/post.h"
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
static gl4f    *colorData;
static gl4f    *gradientColor;

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
static GLuint PRE_BARS_OFFSET;
static GLuint PRE_PROJMATRIX;

// color information
static GLuint PRE_FGCOL;
static GLuint PRE_BGCOL;

// system information providers
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
        bool mirror;
        bool circle;
        bool invert;
    } bars;
    struct circle {
        float start_height;
    } circle;
    struct rotation {
        float per_minute;
        float intensity_scale;
        bool using_intensity;
        bool invert;

        float additional_angle;
    } rotation;
} gl_options;

/**
 * This function is used for handling file change notifications.
 */
EXP_FUNC void xava_gl_module_ionotify_callback(xava_ionotify_event event,
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
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_VERT, "", module, xava);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_FRAG, "", module, xava);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_CONFIG, "", module, xava);

    gl_options.bars.mirror = xavaConfigGetBool(pre.config, "bars", "mirror", false);
    gl_options.bars.circle = xavaConfigGetBool(pre.config, "bars", "circle", false);
    gl_options.bars.invert = xavaConfigGetBool(pre.config, "bars", "invert", false);

    gl_options.circle.start_height =
        xavaConfigGetF64(pre.config, "circle", "start_height", 0.2);

    gl_options.rotation.per_minute =
        xavaConfigGetF64(pre.config, "rotation", "per_minute", 1.0);
    gl_options.rotation.using_intensity =
        xavaConfigGetBool(pre.config, "rotation", "using_intensity", false);
    gl_options.rotation.invert =
        xavaConfigGetBool(pre.config, "rotation", "invert", false);
    gl_options.rotation.intensity_scale =
        xavaConfigGetF64(pre.config, "rotation", "intensity_scale", 1.0);
    gl_options.rotation.additional_angle = 0.0;

    // a little easter egg hehe
    xavaBailCondition(gl_options.rotation.per_minute < 0.0,
            "You cannot time travel, sorry..");

    if(gl_options.bars.circle) {
        xavaBailCondition(xava->conf.autobars == true,
                "Since \"bars_circle\" can't rely on window geometry for bars, "
                "you MUST specify the number of bars in [general]. Sorry about that.");
        xava->conf.flag.ignoreWindowSize = true;
    }
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

    // create programs
    xava_gl_module_program_create(&pre);

    // color
    PRE_FGCOL      = glGetAttribLocation(pre.program, "foreground_color");
    PRE_BGCOL      = glGetUniformLocation(pre.program, "background_color");

    // sys info provider
    PRE_TIME       = glGetUniformLocation(pre.program, "time");
    PRE_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

    // geometry
    PRE_BARS          = glGetAttribLocation( pre.program, "fft_bars");
    PRE_AUDIO         = glGetAttribLocation( pre.program, "audio_data");
    PRE_AUDIO_RATE    = glGetUniformLocation(pre.program, "audio_rate");
    PRE_RESOLUTION    = glGetUniformLocation(pre.program, "resolution");
    PRE_BARS_OFFSET   = glGetUniformLocation(pre.program, "bars_offset");
    PRE_REST          = glGetUniformLocation(pre.program, "rest");
    PRE_BAR_WIDTH     = glGetUniformLocation(pre.program, "bar_width");
    PRE_BAR_SPACING   = glGetUniformLocation(pre.program, "bar_spacing");
    PRE_BAR_COUNT     = glGetUniformLocation(pre.program, "bar_count");
    PRE_PROJMATRIX    = glGetUniformLocation(pre.program, "projection_matrix");

    glUseProgram(pre.program);

    glEnable(GL_DEPTH_TEST);

    // we just need working pointers so that realloc() works
    arr_init(vertexData);
    arr_init(colorData);
    arr_init(gradientColor);

    // gradients
    if(arr_count(conf->gradients) > 0)
        arr_resize(gradientColor, arr_count(conf->gradients));

    for(uint32_t i=0; i<arr_count(conf->gradients); i++) {
        uint32_t grad_col;
        sscanf(conf->gradients[i], "#%x", &grad_col);
        gradientColor[i].r = ARGB_R_32(grad_col) / 255.0;
        gradientColor[i].g = ARGB_G_32(grad_col) / 255.0;
        gradientColor[i].b = ARGB_B_32(grad_col) / 255.0;
        //gradientColor[i].a = ARGB_A_32(grad_col) / 255.0;
        gradientColor[i].a = conf->foreground_opacity;
    }

    shouldRestart = false;
}

#define xava_gl_module_draw_rotate_x(x, y, a) (x*cos(a)-y*sin(a))
#define xava_gl_module_draw_rotate_y(x, y, a) (x*sin(a)+y*cos(a))

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

    glUseProgram(pre.program);

    // reallocate and attach verticies data
    uint32_t segments = MAX(arr_count(conf->gradients), 2)-1;
    size_t   poly_count = xava->bars * 6 * segments;
    if(gl_options.bars.mirror)
        poly_count *= 2;

    arr_resize(vertexData,  poly_count * 2);
    arr_resize(colorData,   poly_count);

    if(gl_options.bars.circle) {
        xava->rest = 0;
    }

    // do image scaling
    projectionMatrix[0] = 2.0/xava->outer.w;
    projectionMatrix[5] = 2.0/xava->outer.h;

    // do image translation
    projectionMatrix[3] = (float)xava->inner.x/xava->outer.w*2.0 - 1.0;
    projectionMatrix[7] = 1.0 - (float)(xava->inner.y+xava->inner.h)/xava->outer.h*2.0;

    glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

    // update screen resoltion
    glUniform2f(PRE_RESOLUTION, xava->inner.w, xava->inner.h);
    glUniform2f(PRE_BARS_OFFSET,
            xava->outer.w - xava->inner.x - xava->inner.w,
            xava->outer.h - xava->inner.y - xava->inner.h);

    // update spacing info
    glUniform1f(PRE_REST,        (float)xava->rest);
    glUniform1f(PRE_BAR_WIDTH,   (float)xava->conf.bw);
    glUniform1f(PRE_BAR_SPACING, (float)xava->conf.bs);
    glUniform1f(PRE_BAR_COUNT,   (float)xava->bars);
    glUniform1f(PRE_AUDIO_RATE,  (float)conf->inputsize);

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
        return; // priority
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

    // Set background color
    GLfloat bgcol[4] = {
        ARGB_R_32(conf->bgcol)/255.0,
        ARGB_G_32(conf->bgcol)/255.0,
        ARGB_B_32(conf->bgcol)/255.0,
        conf->background_opacity
    };
    glUniform4f(PRE_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);

    uint32_t segments = MAX(arr_count(conf->gradients), 2)-1;
    size_t   poly_count = xava->bars * 6 * segments;

    // dont update the colors if the geometry is not ready yet
    if(arr_count(colorData) != poly_count)
        return;

    // Set foreground color(s)
    gl4f fgcol = {
        .r = ARGB_R_32(conf->col)/255.0,
        .g = ARGB_G_32(conf->col)/255.0,
        .b = ARGB_B_32(conf->col)/255.0,
        .a = conf->foreground_opacity
    };
    for(uint32_t j=0; j < segments; j++) {
        for(uint32_t i=0; i<xava->bars; i++) {
            // (bars_done * how_many_segments + segments_done) * num_of_verts_per_bar_segment
            size_t o = (i*segments + j) * 6;

            if(arr_count(conf->gradients) > 1) {
                // top left
                colorData[o]   = gradientColor[j+1];
                // bottom left
                colorData[o+1] = gradientColor[j];
                // bottom right
                colorData[o+2] = gradientColor[j];
                // top right
                colorData[o+3] = gradientColor[j+1];
                // bottom right
                colorData[o+4] = gradientColor[j];
                // top left
                colorData[o+5] = gradientColor[j+1];
            } else {
                for(uint32_t k=0; k<6; k++) {
                    colorData[o+k] = fgcol;
                }
            }
        }
    }

}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    XAVA       *xava  = options->xava;
    XAVA_CONFIG *conf = &xava->conf;

    const float intensity = xava_gl_module_util_calculate_intensity(xava);
    const float time      = xava_gl_module_util_obtain_time();
    const uint32_t segments = MAX(arr_count(gradientColor), 2)-1;
    const float viewport_width =
        gl_options.bars.circle ?
        xava->bars * (conf->bs + conf->bw) :
        xava->inner.w;

    /**
     * Here we start rendering to the texture
     **/
    for(uint32_t j=0; j < segments; j++) {
        float bar_floor = (xava->inner.h / segments) * j;
        float bar_ceil  = (xava->inner.h / segments) * (j+1);
        if(gl_options.bars.mirror) {
            bar_floor /= 2.0;
            bar_ceil  /= 2.0;
            bar_floor += xava->inner.h/2.0;
            bar_ceil += xava->inner.h/2.0;
        }
        for(uint32_t i=0; i<xava->bars; i++) {
            // (bars_done * how_many_segments + segments_done) * num_of_verts_per_bar_segment * 2 floats
            size_t o = (i*segments+j) * 6 * 2;

            float height = xava->f[i];
            if(gl_options.bars.mirror) {
                height /= 2.0;
                height += xava->inner.h/2.0;
            }
            float top = MIN(bar_ceil, MAX(bar_floor, height));

            float progress = MIN(((float)height - bar_floor)/(bar_ceil-bar_floor), 1.0);

            if(gl_options.bars.invert) {
                // top left
                vertexData[o]    = viewport_width - (xava->rest + i*(conf->bs+conf->bw));
                // bottom right
                vertexData[o+4]  = vertexData[o] - conf->bw;
            } else {
                // top left
                vertexData[o]    = xava->rest + i*(conf->bs+conf->bw);
                // bottom right
                vertexData[o+4]  = vertexData[o] + conf->bw;
            }
            // top left
            vertexData[o+1]  = top;
            // bottom left
            vertexData[o+2]  = vertexData[o];
            vertexData[o+3]  = bar_floor;
            // bottom right
            vertexData[o+5]  = bar_floor;
            // top right
            vertexData[o+6]  = vertexData[o+4];
            vertexData[o+7]  = top;
            // bottom right
            vertexData[o+8]  = vertexData[o+4];
            vertexData[o+9]  = bar_floor;
            // top left
            vertexData[o+10] = vertexData[o];
            vertexData[o+11] = top;

            o = (i*segments + j) * 6;
            if(arr_count(conf->gradients) > 1) {
                // top left
                colorData[o]   = mix(gradientColor[j], gradientColor[j+1], progress);
                // top right
                colorData[o+3] = mix(gradientColor[j], gradientColor[j+1], progress);
                // top left
                colorData[o+5] = mix(gradientColor[j], gradientColor[j+1], progress);
            }
        }
    }

    if(gl_options.bars.mirror) {
        // mirroring the result over the Y axis
        {
            const size_t skip = segments * xava->bars * 6 * 2;
            for(uint32_t i=0; i < segments*xava->bars; i++) {
                size_t o = i * 6 * 2;
                vertexData[skip + o]      = vertexData[o];
                vertexData[skip + o + 1]  = xava->inner.h - vertexData[o + 1];
                vertexData[skip + o + 2]  = vertexData[o + 2];
                vertexData[skip + o + 3]  = xava->inner.h - vertexData[o + 3];
                vertexData[skip + o + 4]  = vertexData[o + 4];
                vertexData[skip + o + 5]  = xava->inner.h - vertexData[o + 5];
                vertexData[skip + o + 6]  = vertexData[o + 6];
                vertexData[skip + o + 7]  = xava->inner.h - vertexData[o + 7];
                vertexData[skip + o + 8]  = vertexData[o + 8];
                vertexData[skip + o + 9]  = xava->inner.h - vertexData[o + 9];
                vertexData[skip + o + 10] = vertexData[o + 10];
                vertexData[skip + o + 11] = xava->inner.h - vertexData[o + 11];
            }
        }
        // copying the buffer for the upper section
        {
            const size_t skip = segments * xava->bars * 6;
            for(uint32_t i=0; i < segments*xava->bars; i++) {
                size_t o = i * 6;
                for(uint32_t j=0; j<6; j++)
                    colorData[skip + o + j] = colorData[o + j];
            }
        }
    }

    // Handle post processing geometry manipulation
    if(gl_options.bars.circle) {
        if(gl_options.rotation.using_intensity == false) {
            gl_options.rotation.additional_angle =
                fmod(2.0*M_PI*xava_gl_module_util_obtain_time()/60.0*
                gl_options.rotation.per_minute, 2.0*M_PI);
            if(gl_options.rotation.invert)
                gl_options.rotation.additional_angle *= -1.0;
        } else {
            gl_options.rotation.additional_angle += xava_gl_module_util_calculate_intensity(xava) *
                0.01 * (gl_options.rotation.invert ? -1.0 : 1.0) *
                gl_options.rotation.intensity_scale;
            gl_options.rotation.additional_angle =
                fmod(gl_options.rotation.additional_angle, 2*M_PI);
        }

        const uint32_t m = gl_options.bars.mirror ? 2 : 1;
        for(uint32_t j=0; j < segments; j++) {
            for(uint32_t i=0; i<xava->bars * m; i++) {
                // (bars_done * how_many_segments + segments_done) * num_of_verts_per_bar_segment * 2 floats
                size_t o = (i*segments+j) * 6 * 2;

                for(uint32_t k=0; k<6; k++) {
                    // X coordinate becomes the angle [0 -> 2*M_PI]
                    float angle     = vertexData[o+k*2] / viewport_width * 2*M_PI + gl_options.rotation.additional_angle;
                    // Y becomes amplitude [0 -> 1.0]
                    float amplitude = vertexData[o+k*2+1] / (float)xava->inner.h
                        * (1.0 - gl_options.circle.start_height) + gl_options.circle.start_height;

                    vertexData[o+k*2]    = xava->inner.w * xava_gl_module_draw_rotate_x(amplitude, amplitude, angle) / 2;
                    vertexData[o+k*2+1]  = xava->inner.h * xava_gl_module_draw_rotate_y(amplitude, amplitude, angle) / 2;
                    vertexData[o+k*2]    += xava->inner.w / 2;
                    vertexData[o+k*2+1]  += xava->inner.h / 2;
                }
            }
        }
    }

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME,      time);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // attribute pointers need to be assigned in order and every time we use
    // OpenGL's useProgram()
    glVertexAttribPointer(PRE_BARS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    glEnableVertexAttribArray(PRE_BARS);
    glVertexAttribPointer(PRE_FGCOL, 4, GL_FLOAT, GL_FALSE,
                      0, colorData);
    glEnableVertexAttribArray(PRE_FGCOL);

    // You use the number of verticies, but not the actual polygon count
    size_t vertex_count = xava->bars * 6 * segments;
    if(gl_options.bars.mirror)
        vertex_count *= 2;

    glDrawArrays(GL_TRIANGLES, 0, vertex_count);

    glDisableVertexAttribArray(PRE_BARS);
    glDisableVertexAttribArray(PRE_FGCOL);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    UNUSED(options);

    // delete both pipelines
    xava_gl_module_program_destroy(&pre);

    if(pre.config != NULL) {
        xavaConfigClose(pre.config);
        pre.config = NULL;
    }

    arr_free(gradientColor);
    arr_free(vertexData);
    arr_free(colorData);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

