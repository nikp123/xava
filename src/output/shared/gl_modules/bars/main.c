#include <stdio.h>
#include <string.h>
#include <math.h>

#include "../../gl_shared.h"

#include "../../../../shared.h"
#include "../../../graphical.h"

// functions needed by something else
void xava_gl_module_clear(XAVAGLModuleOptions *options);

// we don't really need this struct, but it's nice to have (for extensibility)
typedef struct xava_gl_module_program {
    struct shader {
        char *path, *text;
        GLuint handle;
    } frag, vert, geo;
    GLuint     program;
    XAVACONFIG config;
} xava_gl_module_program;
xava_gl_module_program pre, post;

static struct FBO {
    GLuint framebuffer;
    GLuint final_texture;
    GLuint depth_texture;
} FBO;

// shader buffers
static GLfloat *vertexData;
static GLfloat *gradientColor;

// used to adjust the view
static GLfloat projectionMatrix[16] =
    {2.0, 0.0,  0.0, -1.0,
     0.0,  2.0,  0.0, -1.0,
     0.0,  0.0, -1.0, -1.0,
     0.0,  0.0,  0.0,  1.0};

// postVertices
static GLfloat postVertices[] = {
    -1.0f, -1.0f,  // Position 0
     0.0f,  0.0f,  // TexCoord 0
     1.0f, -1.0f,  // Position 1
     1.0f,  0.0f,  // TexCoord 1
     1.0f,  1.0f,  // Position 2
     1.0f,  1.0f,  // TexCoord 2
    -1.0f,  1.0f,  // Position 3
     0.0f,  1.0f}; // TexCoord 3

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

// post shader variables

// geometry info
static GLuint POST_POS;
static GLuint POST_TEXCOORD;
static GLuint POST_RESOLUTION;

// textures
static GLuint POST_TEXTURE;
static GLuint POST_DEPTH;

// system info
static GLuint POST_TIME;
static GLuint POST_INTENSITY;

// color information
static GLuint POST_FGCOL;
static GLuint POST_BGCOL;

// this is hacked in, shut up
static bool shouldRestart;

typedef enum sgl_shader_type {
    SGL_PRE,
    SGL_POST
} sgl_shader_type;

typedef enum sgl_shader_stage {
    SGL_VERT,
    SGL_GEO,
    SGL_FRAG,
    SGL_CONFIG
} sgl_shader_stage;

// renderer hacks and options
struct gl_renderer_options {
    struct color {
        bool blending;
    } color;
    struct shader_options {
        bool post_enabled;
    } shader;
} gl_options;

/**
 * This function is used for handling file change notifications.
 */
static void xava_gl_module_ionotify_callback(XAVA_IONOTIFY_EVENT event,
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

void xava_gl_module_shader_load(
        xava_gl_module_program *program,
        sgl_shader_type type,
        sgl_shader_stage stage,
        const char *name,
        XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava = options->xava;
    RawData *file;
    char *returned_path;
    char file_path[MAX_PATH];
    XAVAIONOTIFYWATCHSETUP a;
    MALLOC_SELF(a, 1);

    xavaLog("%s", options->module_prefix);
    switch(type) {
        case SGL_PRE:
            strcpy(file_path, options->module_prefix);
            break;
        case SGL_POST:
            strcpy(file_path, "gl/shaders/");
            strcat(file_path, name);
            break;
        default:
            xavaBail("A really BIG oopsie happened here!");
            break;
    }

    struct shader* shader;
    switch(stage) {
        case SGL_VERT: {
            strcat(file_path, "/vertex.glsl");
            xavaLogCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
                    file_path, &returned_path) == false,
                    "Failed to load '%s'!", file_path);
            shader = &program->vert;
            break;
        }
        case SGL_GEO: {
            xavaWarn("Avoid using geometry shaders as they are quite demanding");
            strcat(file_path, "/geometry.glsl");
            bool success = xavaFindAndCheckFile(XAVA_FILE_TYPE_OPTIONAL_CONFIG,
                                                file_path,
                                                &returned_path);
            if(!success) {
                xavaLog("Failed to load '%s'!", file_path);
                returned_path = NULL;
            }
            shader = &program->geo;
            break;
        }
        case SGL_FRAG: {
            strcat(file_path, "/fragment.glsl");
            xavaBailCondition(xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG,
                    file_path, &returned_path) == false,
                    "Failed to load '%s'!", file_path);
            shader = &program->frag;
            break;
        }
        case SGL_CONFIG: { // load shader config file
            strcat(file_path, "/config.ini");
            bool success = xavaFindAndCheckFile(XAVA_FILE_TYPE_OPTIONAL_CONFIG,
                                                file_path,
                                                &returned_path);
            if(!success) {
                xavaLog("Failed to load '%s'!", file_path);
                returned_path = NULL;
            }
            break;
        }
    }

    // abort if no file and mark as invalid
    if(returned_path == NULL) {
        switch(stage) {
            case SGL_FRAG:
            case SGL_VERT:
            case SGL_GEO:
                shader->text = NULL;
                shader->path = NULL;
                shader->handle = 0;
                break;
            case SGL_CONFIG:
                program->config = NULL;
                break;
        }
        return;
    }

    // load file
    if(stage == SGL_CONFIG) {
        program->config = xavaConfigOpen(returned_path);

        // add watcher
        a->filename           = returned_path;
        a->id                 = 1; // dont really care tbh
        a->xava               = xava;
        a->ionotify           = xava->ionotify;
        a->xava_ionotify_func = xava_gl_module_ionotify_callback;
        xavaBailCondition(!xavaIONotifyAddWatch(a),
            "xavaIONotifyAddWatch failed!");
    } else {
        shader->path = strdup(returned_path);
        file = xavaReadFile(shader->path);
        shader->text = xavaDuplicateMemory(file->data, file->size);
        xavaCloseFile(file);

        // add watcher
        a->filename           = shader->path;
        a->id                 = 1; // dont really care tbh
        a->xava               = xava;
        a->ionotify           = xava->ionotify;
        a->xava_ionotify_func = xava_gl_module_ionotify_callback;
        xavaBailCondition(!xavaIONotifyAddWatch(a),
            "xavaIONotifyAddWatch failed!");
    }

    // clean escape
    free(returned_path);
}

EXP_FUNC void xava_gl_module_config_load(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava = options->xava;
    XAVACONFIG config = xava->default_config.config;

    char *shader;

    shader = xavaConfigGetString(config, "gl", "post_shader", "default");

    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_VERT, "", options);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_FRAG, "", options);
    xava_gl_module_shader_load(&pre, SGL_PRE, SGL_CONFIG, "", options);
    if(strcmp("none", shader)){
        xava_gl_module_shader_load(&post, SGL_POST, SGL_VERT, shader, options);
        xava_gl_module_shader_load(&post, SGL_POST, SGL_GEO,  shader, options);
        xava_gl_module_shader_load(&post, SGL_POST, SGL_FRAG, shader, options);
        gl_options.shader.post_enabled = true;
    } else {
        gl_options.shader.post_enabled = false;
    }

    // read options from the "pre" shader
    if(pre.config != NULL) {
        gl_options.color.blending = xavaConfigGetBool(
                pre.config, "color", "blending", false);
        xavaConfigClose(pre.config);
    } else {
        gl_options.color.blending = false;
    }
}

GLint xava_gl_module_shader_build(struct shader *shader, GLenum shader_type) {
    GLint status;

    shader->handle = glCreateShader(shader_type);
    xavaReturnErrorCondition(shader->handle == 0, 0, "Failed to build shader");

    glShaderSource(shader->handle, 1, (const char **) &shader->text, NULL);
    glCompileShader(shader->handle);

    glGetShaderiv(shader->handle, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1000] = {0};
        GLsizei len;
        glGetShaderInfoLog(shader->handle, 1000, &len, log);

        bool unknown_type = false;
        int file, line, char_num;
        size_t matched_elements = sscanf(log, "%d:%d(%d)",
                &file, &line, &char_num);
        if(matched_elements < 3) {
            size_t matched_elements = sscanf(log, "%d(%d)",
                    &line, &char_num);
            if(matched_elements < 2)
                unknown_type = true;
        }

        if(unknown_type) {
            xavaError("Error: Compiling '%s' failed\n%*s",
                    shader->path, len, log);
        } else {
            char *source_line;
            char *string_ptr = shader->text;
            for(int i=0; i < line-1; i++) {
                string_ptr = strchr(string_ptr, '\n');
                string_ptr++;
            }
            source_line = strdup(string_ptr);
            *strchr(source_line, '\n') = '\0';

            xavaError("Error: Compiling '%s' failed\n%.*sCode:\n% 4d: %.*s",
                    shader->path, len, log, line, 256, source_line);

            free(source_line);
        }

        return status;
    }

    xavaSpam("Compiling '%s' successful", shader->path);

    // free text and path info
    free(shader->path);
    free(shader->text);

    return status;
}

void xava_gl_module_program_create(xava_gl_module_program *program) {
    GLint status;

    program->program = glCreateProgram();
    xava_gl_module_shader_build(&program->vert,    GL_VERTEX_SHADER);
    if(program->geo.text) // optional stage, we check if it's included in the shader pack
        xava_gl_module_shader_build(&program->geo, GL_GEOMETRY_SHADER);
    xava_gl_module_shader_build(&program->frag,    GL_FRAGMENT_SHADER);

    glAttachShader(program->program, program->vert.handle);
    if(program->geo.text) // optional stage, we check if it's included in the shader pack
        glAttachShader(program->program, program->geo.handle);
    glAttachShader(program->program, program->frag.handle);
    glLinkProgram(program->program);

    glDeleteShader(program->frag.handle);
    if(program->geo.text) // optional stage, we check if it's included in the shader pack
        glDeleteShader(program->geo.handle);
    glDeleteShader(program->vert.handle);

    glGetProgramiv(program->program, GL_LINK_STATUS, &status);
    if (!status) {
        char log[1000];
        GLsizei len;
        glGetProgramInfoLog(program->program, 1000, &len, log);
        xavaBail("Error: linking:\n%*s\n", len, log);
    }
}

EXP_FUNC void xava_gl_module_init(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE *xava = options->xava;
    struct config_params *conf = &xava->conf;

    // automatically assign this so it isn't invalid during framebuffer creation
    xava->w = conf->w;
    xava->h = conf->h;

    xava_gl_module_program_create(&pre);
    if(gl_options.shader.post_enabled)
        xava_gl_module_program_create(&post);

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

    if(gl_options.shader.post_enabled) {
        POST_POS        = glGetAttribLocation(post.program,  "v_texCoord");
        POST_TEXCOORD   = glGetAttribLocation(post.program,  "m_texCoord");
        POST_TEXTURE    = glGetUniformLocation(post.program, "color_texture");
        POST_DEPTH      = glGetUniformLocation(post.program, "depth_texture");
        POST_TIME       = glGetUniformLocation(post.program, "time");
        POST_INTENSITY  = glGetUniformLocation(post.program, "intensity");
        POST_RESOLUTION = glGetUniformLocation(post.program,  "resolution");

        POST_FGCOL     = glGetUniformLocation(post.program, "foreground_color");
        POST_BGCOL     = glGetUniformLocation(post.program, "background_color");
    }

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

    if(gl_options.shader.post_enabled == false) return;

    glUseProgram(post.program);

    // update screen resoltion
    glUniform2f(POST_RESOLUTION, xava->w, xava->h);

    // set texture properties
    glGenTextures(1,               &FBO.final_texture);
    glBindTexture(GL_TEXTURE_2D,   FBO.final_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xava->w*options->resolution_scale, xava->h*options->resolution_scale,
            0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set texture properties
    glGenTextures(1,      &FBO.depth_texture);
    glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, xava->w*options->resolution_scale,
            xava->h*options->resolution_scale, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set framebuffer properties
    glGenFramebuffers(1,  &FBO.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, FBO.final_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, FBO.depth_texture, 0);

    // check if it borked
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    xavaBailCondition(status != GL_FRAMEBUFFER_COMPLETE,
            "Failed to create framebuffer(s)! Error code 0x%X\n",
            status);
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

    // set and attach foreground color
    float fgcol[4] = {
        ARGB_R_32(conf->col)/255.0,
        ARGB_G_32(conf->col)/255.0,
        ARGB_B_32(conf->col)/255.0,
        conf->foreground_opacity
    };
    glUniform4f(PRE_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // set background clear color
    float bgcol[4] = {
        ARGB_R_32(conf->bgcol)/255.0,
        ARGB_G_32(conf->bgcol)/255.0,
        ARGB_B_32(conf->bgcol)/255.0,
        conf->background_opacity
    };
    glUniform4f(PRE_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);

    if(gl_options.shader.post_enabled == false) return;

    glUseProgram(post.program);

    // set and attach foreground color
    glUniform4f(POST_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // set background clear color
    glUniform4f(POST_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

EXP_FUNC void xava_gl_module_draw(XAVAGLModuleOptions *options) {
    struct XAVA_HANDLE   *xava  = options->xava;
    struct config_params *conf  = &xava->conf;

    // restrict time variable to one hour because floating point precision issues
    float currentTime = (float)fmodl((long double)xavaGetTime()/(long double)1000.0, 3600.0);
    float intensity = 0.0;

    /**
     * Here we start rendering to the texture
     **/

    register GLfloat *d = vertexData;
    for(register int i=0; i<xava->bars; i++) {
        // the not so, speed part
        // intensity has a low-freq bias as they are more "physical"
        float bar_percentage = (float)(xava->f[i]-1)/(float)conf->h;
        if(bar_percentage > 0.0) {
            intensity+=powf(bar_percentage, (float)2.0*(float)i/(float)xava->bars);
        }

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

    // since im not bothering to do the math, this'll do
    // - used to balance out intensity across various number of bars
    intensity /= xava->bars;

    // bind render target to texture
    glBindFramebuffer(GL_FRAMEBUFFER, 
            gl_options.shader.post_enabled ? FBO.framebuffer : 0);
    if(gl_options.shader.post_enabled)
        glViewport(0, 0, xava->w*options->resolution_scale, xava->h*options->resolution_scale);
    else
        glViewport(0, 0, xava->w, xava->h);

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME, currentTime);

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
    if(gl_options.shader.post_enabled == false) return;

    /**
     * Once the texture has been conpleted, we now activate a seperate pipeline
     * which just displays that texture to the actual framebuffer for easier
     * shader writing
     * */

    // Change framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, xava->w, xava->h);

    // Switch to post shaders
    glUseProgram(post.program);

    // update time
    glUniform1f(POST_TIME, currentTime);

    // update intensity
    glUniform1f(POST_INTENSITY, intensity);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // set attribute array pointers for the texture unit and vertex unit
    glVertexAttribPointer(POST_POS, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat), postVertices);
    glVertexAttribPointer(POST_TEXCOORD, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat), &postVertices[2]);

    // enable the use of the following attribute elements
    glEnableVertexAttribArray(POST_POS);
    glEnableVertexAttribArray(POST_TEXCOORD);

    // Bind the textures
    // Render texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, FBO.final_texture);
    glUniform1i(POST_TEXTURE, 0);

    // Depth texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
    glUniform1i(POST_DEPTH, 1);

    // draw frame
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    glDisableVertexAttribArray(POST_POS);
    glDisableVertexAttribArray(POST_TEXCOORD);
}

void xava_gl_module_program_destroy(xava_gl_module_program *program) {
    glDeleteProgram(program->program);
}

EXP_FUNC void xava_gl_module_cleanup(XAVAGLModuleOptions *options) {
    // delete both pipelines
    xava_gl_module_program_destroy(&pre);
    xava_gl_module_program_destroy(&post);

    free(gradientColor);
    free(vertexData);
}

EXP_FUNC xava_version xava_gl_module_version(void) {
    return xava_version_get();
}

