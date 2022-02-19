#include <stdio.h>
#include <string.h>
#include <math.h>

#include "output/shared/gl/util/shader.h"
//#include "output/shared/gl/modules/shared/post.h"
#include "output/shared/gl/util/misc.h"
#include "output/shared/gl/main.h"

#include "shared.h"
#include "output/shared/graphical.h"

struct star {
    float x, y;
    float angle;
    uint32_t size;
} *stars;

struct star_options {
    float    density;
    uint32_t count;
    uint32_t max_size;
    char     *color_str;
    uint32_t color;
    bool     depth_test;
} star;

// functions needed by something else
void xava_gl_module_clear(XAVAGLModuleOptions *options);

// we don't really need this struct, but it's nice to have (for extensibility)
xava_gl_module_program pre;

// shader buffers
static GLfloat *vertexData;
static GLfloat *colorData;

// used to adjust the view
static GLfloat projectionMatrix[16] =
    {2.0, 0.0,  0.0, -1.0,
     0.0,  2.0,  0.0, -1.0,
     0.0,  0.0, -1.0, -1.0,
     0.0,  0.0,  0.0,  1.0};

// geometry information
static GLuint SHADER_POS;
static GLuint PRE_RESOLUTION;
static GLuint PRE_PROJMATRIX;

// color information
static GLuint SHADER_COLOR;
static GLuint PRE_FGCOL;
static GLuint PRE_BGCOL;

// system information providers
static GLuint PRE_TIME;
static GLuint PRE_INTENSITY;

// this is hacked in, shut up
static bool shouldRestart;

typedef struct color4f {
    GLfloat r, g, b, a;
} color4f;

typedef struct vec2f {
    GLfloat x, y;
} vec2f;

// star functions
float xava_generate_star_angle(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return 0.7 - pow(sin(r*M_PI), 0.5);
}

uint32_t xava_generate_star_size(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return floor((1.0-pow(r, 0.5))*star.max_size)+1;
}

/**
 * This function is used for handling file change notifications.
 */
EXP_FUNC void xava_gl_module_ionotify_callback(XAVA_IONOTIFY_EVENT event,
                                const char *filename,
                                int id,
                                XAVA* xava) {
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

    star.count     = xavaConfigGetInt(pre.config, "stars", "count", 0);
    star.density   = 0.0001 * xavaConfigGetDouble(pre.config, "stars", "density", 1.0);
    star.max_size  = xavaConfigGetInt(pre.config, "stars", "max_size", 5);
    star.color_str = xavaConfigGetString(pre.config, "stars", "color", NULL);
    star.depth_test = xavaConfigGetBool(pre.config, "stars", "depth_test", false);

    xavaBailCondition(star.max_size < 1, "max_size cannot be below 1");
    xavaBailCondition(star.count < 0, "star count cannot be negative");

}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    // create programs
    xava_gl_module_program_create(&pre);

    // color
    PRE_FGCOL      = glGetUniformLocation(pre.program, "foreground_color");
    PRE_BGCOL      = glGetUniformLocation(pre.program, "background_color");

    // sys info provider
    PRE_TIME       = glGetUniformLocation(pre.program, "time");
    PRE_INTENSITY  = glGetUniformLocation(pre.program, "intensity");

    // geometry
    SHADER_POS     = glGetAttribLocation( pre.program, "pos");
    SHADER_COLOR   = glGetAttribLocation( pre.program, "color");
    PRE_RESOLUTION = glGetUniformLocation(pre.program, "resolution");
    PRE_PROJMATRIX = glGetUniformLocation(pre.program, "projection_matrix");

    glUseProgram(pre.program);

    // we just need working pointers so that realloc() works
    arr_init(vertexData);
    arr_init(colorData);
    arr_init(stars);

    shouldRestart = false;
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;

    glUseProgram(pre.program);

    // reallocate and attach verticies data
    int32_t star_count;
    if(star.count == 0) {
        // very scientific, much wow
        star_count = xava->outer.w*xava->outer.h*star.density;
    } else {
        star_count = star.count;
    }

    if(star.color_str == NULL) {
        star.color = xava->conf.col;

        // this is dumb, but it works
        star.color |= ((uint8_t)xava->conf.foreground_opacity*0xFF)<<24;
    } else do {
        int err = sscanf(star.color_str,
                "#%08x", &star.color);
        if(err == 1)
            break;

        err = sscanf(star.color_str,
                "#%08X", &star.color);
        if(err == 1)
            break;

        xavaBail("'%s' is not a valid color", star.color_str);
    } while(0);

    arr_resize(stars, star_count);
    arr_resize(vertexData, star_count*12);
    arr_resize(colorData, star_count*4*6);
    glVertexAttribPointer(SHADER_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    glVertexAttribPointer(SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, colorData);

    // since most of this information remains untouched, let's precalculate
    for(int i=0; i<star_count; i++) {
        float l = stars[i].x;
        float r = l + stars[i].size;
        float b = stars[i].y;
        float t = b + stars[i].size;

        // generate the stars with random angles
        // but with a bias towards the right
        stars[i].angle = xava_generate_star_angle();
        stars[i].x     = fmod(rand(), xava->outer.w);
        stars[i].y     = fmod(rand(), xava->outer.h);
        stars[i].size  = xava_generate_star_size();

        ((vec2f*)vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+5] = (vec2f){l, t};

        color4f color = {
            ARGB_R_32(star.color)/255.0,
            ARGB_G_32(star.color)/255.0,
            ARGB_B_32(star.color)/255.0,
            1.0 - ((GLfloat)stars[i].size-1)/star.max_size
        };

        ((color4f*)colorData)[i*6+0] = color;
        ((color4f*)colorData)[i*6+1] = color;
        ((color4f*)colorData)[i*6+2] = color;
        ((color4f*)colorData)[i*6+3] = color;
        ((color4f*)colorData)[i*6+4] = color;
        ((color4f*)colorData)[i*6+5] = color;
    }

    // do image scaling
    projectionMatrix[0] = 2.0/xava->outer.w;
    projectionMatrix[5] = 2.0/xava->outer.h;

    // do image translation
    //projectionMatrix[3] = (float)xava->inner.x/xava->outer.w*2.0 - 1.0;
    //projectionMatrix[7] = 1.0 - (float)(xava->inner.y+xava->inner.h)/xava->outer.h*2.0;

    glUniformMatrix4fv(PRE_PROJMATRIX, 1, GL_FALSE, (GLfloat*) projectionMatrix);

    // update screen resoltion
    glUniform2f(PRE_RESOLUTION, xava->outer.w, xava->outer.h);

    // "clear" the screen
    xava_gl_module_clear(options);
}

EXP_FUNC XG_EVENT xava_gl_module_event(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;

    if(shouldRestart) {
        shouldRestart = false;
        return XAVA_RELOAD;
    }

    // check if the visualizer bounds were changed
    if((xava->inner.w != xava->bar_space.w) ||
       (xava->inner.h != xava->bar_space.h)) {
        xava->bar_space.w = xava->inner.w;
        xava->bar_space.h = xava->inner.h;
        return XAVA_RESIZE;
    }

    return XAVA_IGNORE;
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
        ARGB_R_32(star.color)/255.0,
        ARGB_G_32(star.color)/255.0,
        ARGB_B_32(star.color)/255.0,
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

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    XAVA   *xava  = options->xava;

    float intensity = xava_gl_module_util_calculate_intensity(xava);
    float time      = xava_gl_module_util_obtain_time();

    glEnable(GL_BLEND);

    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    /**
     * Here we start rendering to the texture
     **/
    for(register int i=0; i<arr_count(stars); i++) {
        stars[i].x += stars[i].size*cos(stars[i].angle)*intensity;
        stars[i].y += stars[i].size*sin(stars[i].angle)*intensity;

        if(stars[i].x < 0.0-stars[i].size) {
            stars[i].x = xava->outer.w;
            stars[i].angle = xava_generate_star_angle();
        } else if(stars[i].x > xava->outer.w+stars[i].size) {
            stars[i].x = 0;
            stars[i].angle = xava_generate_star_angle();
        }

        if(stars[i].y < 0.0-stars[i].size) {
            stars[i].y = xava->outer.h;
            stars[i].angle = xava_generate_star_angle();
        } else if(stars[i].y > xava->outer.h+stars[i].size) {
            stars[i].y = 0;
            stars[i].angle = xava_generate_star_angle();
        }

        float l = stars[i].x;
        float r = l + stars[i].size;
        float b = stars[i].y;
        float t = b + stars[i].size;

        ((vec2f*)vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)vertexData)[i*6+5] = (vec2f){l, t};
    }

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME,      time);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(SHADER_POS, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
    glEnableVertexAttribArray(SHADER_POS);

    glVertexAttribPointer(SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, colorData);
    glEnableVertexAttribArray(SHADER_COLOR);

    if(star.depth_test == false)
        glDisable(GL_DEPTH_TEST);

    // You use the number of verticies, but not the actual polygon count
    glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

    // other modules imply the usage of the depth buffer, so we're going to
    // enable it for them
    glEnable(GL_DEPTH_TEST);

    glDisableVertexAttribArray(SHADER_POS);
    glDisableVertexAttribArray(SHADER_COLOR);

    // disable blending on the post stage as it produces
    // invalid colors on the window manager end
    glDisable(GL_BLEND);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    // delete both pipelines
    xava_gl_module_program_destroy(&pre);

    arr_free(vertexData);
    arr_free(colorData);
    arr_free(stars);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

