#include <stdio.h>
#include <string.h>
#include <math.h>

#include "output/shared/gl/util/shader.h"
//#include "output/shared/gl/modules/shared/post.h"
#include "output/shared/gl/util/misc.h"
#include "output/shared/gl/main.h"

#include "shared.h"
#include "output/shared/graphical.h"

// define types
struct star {
    float     x, y;
    float    angle;
    uint32_t size;
};

struct star_options {
    float    density;
    uint32_t count;
    uint32_t max_size;
    char     *color_str;
    uint32_t color;
    bool     depth_test;
};

typedef struct color4f {
    GLfloat r, g, b, a;
} color4f;

typedef struct vec2f {
    GLfloat x, y;
} vec2f;

// functions needed by something else
void xava_gl_module_clear(XAVAGLModuleOptions *options);

struct star_data {
    struct star *stars;
    struct star_options options;

    // we don't really need this struct, but it's nice to have (for extensibility)
    xava_gl_module_program pre;

    // shader buffers
    GLfloat *vertexData;
    GLfloat *colorData;

    // used to adjust the view
    GLfloat projectionMatrix[16];

    // geometry information
    GLuint SHADER_POS;
    GLuint PRE_RESOLUTION;
    GLuint PRE_PROJMATRIX;

    // color information
    GLuint SHADER_COLOR;
    GLuint PRE_FGCOL;
    GLuint PRE_BGCOL;

    // system information providers
    GLuint PRE_TIME;
    GLuint PRE_INTENSITY;

    // this is hacked in, shut up
    bool shouldRestart;
};

// star functions
float xava_generate_star_angle(void) {
    float r = (float)rand()/(float)RAND_MAX;

    return 0.7 - pow(sin(r*M_PI), 0.5);
}

float xava_generate_star_size(struct star_options *options) {
    float r = (float)rand()/(float)RAND_MAX;

    return (1.0-powf(r, 0.5))*options->max_size+1.0;
}

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
            // This paradigm sucks.
            //shouldRestart = true;
            break;
        default:
            // noop
            break;
    }
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModule *module, XAVA *xava) {
    // keep module data inside of a struct because UB otherwise
    module->options.data = calloc(1, sizeof(struct star_data));

    struct star_data *data = module->options.data;

    // Initialize the projection matrix
    // God, C sucks balls
    memcpy(data->projectionMatrix,
       (GLfloat[16]){
           2.0f, 0.0f,  0.0f, -1.0f,
           0.0f, 2.0f,  0.0f, -1.0f,
           0.0f, 0.0f, -1.0f, -1.0f,
           0.0f, 0.0f,  0.0f,  1.0f
       },
       sizeof data->projectionMatrix);

    xava_gl_module_shader_load(&data->pre, SGL_PRE, SGL_VERT, "", module, xava);
    xava_gl_module_shader_load(&data->pre, SGL_PRE, SGL_FRAG, "", module, xava);
    xava_gl_module_shader_load(&data->pre, SGL_PRE, SGL_CONFIG, "", module, xava);

    data->options.count     = xavaConfigGetI32(data->pre.config, "stars", "count", 0);
    data->options.density   = 0.0001 * xavaConfigGetF64(data->pre.config, "stars", "density", 1.0);
    data->options.max_size  = xavaConfigGetI32(data->pre.config, "stars", "max_size", 5);
    data->options.color_str = xavaConfigGetString(data->pre.config, "stars", "color", NULL);
    data->options.depth_test = xavaConfigGetBool(data->pre.config, "stars", "depth_test", false);

    xavaBailCondition(data->options.max_size < 1, "max_size cannot be below 1");
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    struct star_data *data = options->data;

    // create programs
    xava_gl_module_program_create(&data->pre);

    glUseProgram(data->pre.program);

    // color
    data->PRE_FGCOL      = glGetUniformLocation(data->pre.program, "foreground_color");
    data->PRE_BGCOL      = glGetUniformLocation(data->pre.program, "background_color");

    // sys info provider
    data->PRE_TIME       = glGetUniformLocation(data->pre.program, "time");
    data->PRE_INTENSITY  = glGetUniformLocation(data->pre.program, "intensity");

    // geometry
    data->SHADER_POS     = glGetAttribLocation( data->pre.program, "pos");
    data->SHADER_COLOR   = glGetAttribLocation( data->pre.program, "color");
    data->PRE_RESOLUTION = glGetUniformLocation(data->pre.program, "resolution");
    data->PRE_PROJMATRIX = glGetUniformLocation(data->pre.program, "projection_matrix");

    // we just need working pointers so that realloc() works
    arr_init(data->vertexData);
    arr_init(data->colorData);
    arr_init(data->stars);

    data->shouldRestart = false;
}

EXP_FUNC void xava_gl_module_apply(XAVAGLModuleOptions *options) {
    XAVA *xava = options->xava;
    struct star_data *data = options->data;

    glUseProgram(data->pre.program);

    // reallocate and attach verticies data
    uint32_t star_count;
    if(data->options.count == 0) {
        // very scientific, much wow
        star_count = xava->outer.w*xava->outer.h*data->options.density;
    } else {
        star_count = data->options.count;
    }

    if(data->options.color_str == NULL) {
        data->options.color = xava->conf.col;

        // this is dumb, but it works
        data->options.color |= ((uint8_t)xava->conf.foreground_opacity*0xFF)<<24;
    } else do {
        int err = sscanf(data->options.color_str,
                "#%08x", &data->options.color);
        if(err == 1)
            break;

        err = sscanf(data->options.color_str,
                "#%08X", &data->options.color);
        if(err == 1)
            break;

        xavaBail("'%s' is not a valid color", data->options.color_str);
    } while(0);

    arr_resize(data->stars,      star_count);
    arr_resize(data->vertexData, star_count*6*2);
    arr_resize(data->colorData,  star_count*6*4);
    glVertexAttribPointer(data->SHADER_POS,   2, GL_FLOAT, GL_FALSE, 0, data->vertexData);
    glVertexAttribPointer(data->SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, data->colorData);

    // since most of this information remains untouched, let's precalculate
    for(uint32_t i=0; i<star_count; i++) {
        // generate the stars with random angles
        // but with a bias towards the right
        data->stars[i].angle = xava_generate_star_angle();
        data->stars[i].x     = fmod(rand(), xava->outer.w);
        data->stars[i].y     = fmod(rand(), xava->outer.h);
        data->stars[i].size  = xava_generate_star_size(&data->options);

        float l = floor(data->stars[i].x);
        float r = l + data->stars[i].size;
        float b = floor(data->stars[i].y);
        float t = b + data->stars[i].size;

        ((vec2f*)data->vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)data->vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)data->vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)data->vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)data->vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)data->vertexData)[i*6+5] = (vec2f){l, t};

        color4f color = {
            ARGB_R_32(data->options.color)/255.0,
            ARGB_G_32(data->options.color)/255.0,
            ARGB_B_32(data->options.color)/255.0,
            1.0 - (data->stars[i].size-1.0)/data->options.max_size
        };

        ((color4f*)data->colorData)[i*6+0] = color;
        ((color4f*)data->colorData)[i*6+1] = color;
        ((color4f*)data->colorData)[i*6+2] = color;
        ((color4f*)data->colorData)[i*6+3] = color;
        ((color4f*)data->colorData)[i*6+4] = color;
        ((color4f*)data->colorData)[i*6+5] = color;
    }

    // do image scaling
    data->projectionMatrix[0] = 2.0/xava->outer.w;
    data->projectionMatrix[5] = 2.0/xava->outer.h;

    // do image translation
    //data->projectionMatrix[3] = (float)xava->inner.x/xava->outer.w*2.0 - 1.0;
    //data->projectionMatrix[7] = 1.0 - (float)(xava->inner.y+xava->inner.h)/xava->outer.h*2.0;

    glUniformMatrix4fv(data->PRE_PROJMATRIX, 1, GL_FALSE,
            (GLfloat*) data->projectionMatrix);

    // update screen resoltion
    glUniform2f(data->PRE_RESOLUTION, xava->outer.w, xava->outer.h);

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
    XAVA *xava             = options->xava;
    struct star_data *data = options->data;
    XAVA_CONFIG *conf      = &xava->conf;

    // if you want to fiddle with certain uniforms from a shader, YOU MUST SWITCH TO IT
    // (https://www.khronos.org/opengl/wiki/GLSL_:_common_mistakes#glUniform_doesn.27t_work)
    glUseProgram(data->pre.program);
}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    XAVA   *xava      = options->xava;
    struct star_data *data = options->data;

    float intensity = xava_gl_module_util_calculate_intensity(xava);
    float time      = xava_gl_module_util_obtain_time();

    /**
     * Here we start rendering to the texture
     **/
    for(register uint32_t i=0; i<arr_count(data->stars); i++) {
        struct star *star = &data->stars[i];

        star->x += star->size*cos(star->angle)*intensity;
        star->y += star->size*sin(star->angle)*intensity;

        if(star->x < 0.0-star->size) {
            star->x = xava->outer.w;
            star->angle = xava_generate_star_angle();
        } else if(star->x > xava->outer.w+star->size) {
            star->x = 0;
            star->angle = xava_generate_star_angle();
        }

        if(star->y < 0.0-star->size) {
            star->y = xava->outer.h;
            star->angle = xava_generate_star_angle();
        } else if(star->y > xava->outer.h+star->size) {
            star->y = 0;
            star->angle = xava_generate_star_angle();
        }

        float l = star->x;
        float r = l + star->size;
        float b = star->y;
        float t = b + star->size;

        ((vec2f*)data->vertexData)[i*6+0] = (vec2f){l, t};
        ((vec2f*)data->vertexData)[i*6+1] = (vec2f){l, b};
        ((vec2f*)data->vertexData)[i*6+2] = (vec2f){r, b};
        ((vec2f*)data->vertexData)[i*6+3] = (vec2f){r, t};
        ((vec2f*)data->vertexData)[i*6+4] = (vec2f){r, b};
        ((vec2f*)data->vertexData)[i*6+5] = (vec2f){l, t};
    }

    // switch to pre shaders
    glUseProgram(data->pre.program);

    // update time
    glUniform1f(data->PRE_TIME,      time);

    // update intensity
    glUniform1f(data->PRE_INTENSITY, intensity);

    // update colors lol
    float fgcol[4] = {
        ARGB_R_32(data->options.color)/255.0,
        ARGB_G_32(data->options.color)/255.0,
        ARGB_B_32(data->options.color)/255.0,
        1.0
    };

    glUniform4f(data->PRE_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(data->SHADER_POS, 2, GL_FLOAT, GL_FALSE, 0, data->vertexData);
    glEnableVertexAttribArray(data->SHADER_POS);

    glVertexAttribPointer(data->SHADER_COLOR, 4, GL_FLOAT, GL_FALSE, 0, data->colorData);
    glEnableVertexAttribArray(data->SHADER_COLOR);

    if(data->options.depth_test == false)
        glDisable(GL_DEPTH_TEST);

    // You use the number of verticies, but not the actual polygon count
    glDrawArrays(GL_TRIANGLES, 0, xava->bars*6);

    // other modules imply the usage of the depth buffer, so we're going to
    // enable it for them
    glEnable(GL_DEPTH_TEST);

    glDisableVertexAttribArray(data->SHADER_POS);
    glDisableVertexAttribArray(data->SHADER_COLOR);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    struct star_data *data = options->data;

    // delete both pipelines
    xava_gl_module_program_destroy(&data->pre);

    arr_free(data->vertexData);
    arr_free(data->colorData);
    arr_free(data->stars);

    // dont free as it config_load is not rerun on refresh
    free(options->data);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

