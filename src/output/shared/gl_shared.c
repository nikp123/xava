#include <stdio.h>
#include <string.h>
#include <math.h>

#include "gl_shared.h"

#include "../../shared.h"
#include "../graphical.h"

// we don't really need this struct, but it's nice to have (for extensibility)
struct SGLprogram {
    struct shader {
        char *path, *text;
        GLuint handle;
    } frag, vert, geo;
    GLuint     program;
    XAVACONFIG config;
} pre, post;

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

static GLfloat shaderMemory[4096];

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

// GPU dark magic
static GLuint POST_SSBO;

// special
static GLfloat resScale;

// this is hacked in, shut up
static bool shouldRestart;

static void ionotify_callback(XAVA_IONOTIFY_EVENT event,
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

enum sgl_shader_type {
    SGL_PRE,
    SGL_POST
};

enum sgl_shader_stage {
    SGL_VERT,
    SGL_GEO,
    SGL_FRAG,
    SGL_CONFIG
};

// renderer hacks and options
struct gl_renderer_options {
    bool use_fft;
    bool demand_stereo;
    bool enable_blending;
    GLuint line_width;
} gl_options;

void internal_SGLLoadShader(struct SGLprogram *program,
        enum sgl_shader_type type,
        enum sgl_shader_stage stage,
        const char *name,
        struct XAVA_HANDLE *xava) {
    RawData *file;
    char *returned_path;
    char file_path[MAX_PATH];
    XAVAIONOTIFYWATCHSETUP a;
    MALLOC_SELF(a, 1);

    switch(type) {
        case SGL_PRE:
            strcpy(file_path, "gl/shaders/pre/");
            break;
        case SGL_POST:
            strcpy(file_path, "gl/shaders/post/");
            break;
        default:
            xavaBail("A really BIG oopsie happened here!");
            break;
    }

    strcat(file_path, name);

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
        a->xava_ionotify_func = ionotify_callback;
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
        a->xava_ionotify_func = ionotify_callback;
        xavaBailCondition(!xavaIONotifyAddWatch(a),
            "xavaIONotifyAddWatch failed!");
    }

    // clean escape
    free(returned_path);
}

void SGLConfigLoad(struct XAVA_HANDLE *xava) {
    XAVACONFIG config = xava->default_config.config;
    struct config_params *p = &xava->conf;

    char *prePack, *postPack;

    prePack  = xavaConfigGetString(config, "gl", "pre_shaderpack", "default");
    postPack = xavaConfigGetString(config, "gl", "post_shaderpack", "default");

    resScale    = xavaConfigGetDouble(config, "gl", "resolution_scale", 1.0f);

    internal_SGLLoadShader(&pre, SGL_PRE, SGL_VERT, prePack, xava);
    internal_SGLLoadShader(&pre, SGL_PRE, SGL_GEO,  prePack, xava);
    internal_SGLLoadShader(&pre, SGL_PRE, SGL_FRAG, prePack, xava);
    internal_SGLLoadShader(&pre, SGL_PRE, SGL_CONFIG, prePack, xava);
    internal_SGLLoadShader(&post, SGL_POST, SGL_VERT, postPack, xava);
    internal_SGLLoadShader(&post, SGL_POST, SGL_GEO,  postPack, xava);
    internal_SGLLoadShader(&post, SGL_POST, SGL_FRAG, postPack, xava);

    // read options from the "pre" shader
    if(pre.config != NULL) {
        gl_options.use_fft         = xavaConfigGetBool(
                pre.config, "display", "use_fft", true);
        gl_options.demand_stereo   = xavaConfigGetBool(
                pre.config, "display", "demand_stereo", false);
        gl_options.enable_blending = xavaConfigGetBool(
                pre.config, "display", "enable_blending", false);
        gl_options.line_width      = xavaConfigGetInt(
                pre.config, "display", "line_width", false);
        xavaConfigClose(pre.config);
    } else {
        gl_options.use_fft         = true;
        gl_options.demand_stereo   = false;
        gl_options.enable_blending = false;
        gl_options.line_width      = 1;
    }

    // parse options from the "pre" shader
    p->skipFilterF = !gl_options.use_fft;
    if(!gl_options.use_fft)
        p->inputsize = p->samplerate / p->framerate;
    p->stereo |= gl_options.demand_stereo;
}

GLint SGLShaderBuild(struct shader *shader, GLenum shader_type) {
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

void SGLCreateProgram(struct SGLprogram *program) {
    GLint status;

    program->program = glCreateProgram();
    SGLShaderBuild(&program->vert,    GL_VERTEX_SHADER);
    if(program->geo.text) // optional stage, we check if it's included in the shader pack
        SGLShaderBuild(&program->geo, GL_GEOMETRY_SHADER);
    SGLShaderBuild(&program->frag,    GL_FRAGMENT_SHADER);

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

void SGLInit(struct XAVA_HANDLE *xava) {
    struct config_params *conf = &xava->conf;

    // automatically assign this so it isn't invalid during framebuffer creation
    xava->w = conf->w;
    xava->h = conf->h;

    SGLCreateProgram(&pre);
    SGLCreateProgram(&post);

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

    POST_POS        = glGetAttribLocation(post.program,  "v_texCoord");
    POST_TEXCOORD   = glGetAttribLocation(post.program,  "m_texCoord");
    POST_TEXTURE    = glGetUniformLocation(post.program, "color_texture");
    POST_DEPTH      = glGetUniformLocation(post.program, "depth_texture");
    POST_TIME       = glGetUniformLocation(post.program, "time");
    POST_INTENSITY  = glGetUniformLocation(post.program, "intensity");
    POST_RESOLUTION = glGetUniformLocation(post.program,  "resolution");

    POST_FGCOL     = glGetUniformLocation(post.program, "foreground_color");
    POST_BGCOL     = glGetUniformLocation(post.program, "background_color");

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

    glEnable(GL_LINE_SMOOTH);
    glLineWidth(gl_options.line_width);
}

void SGLApply(struct XAVA_HANDLE *xava){
    struct config_params *conf = &xava->conf;

    glUseProgram(post.program);

    // update screen resoltion
    glUniform2f(POST_RESOLUTION, xava->w, xava->h);

    // set texture properties
    glGenTextures(1,               &FBO.final_texture);
    glBindTexture(GL_TEXTURE_2D,   FBO.final_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, xava->w*resScale, xava->h*resScale,
            0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set texture properties
    glGenTextures(1,      &FBO.depth_texture);
    glBindTexture(GL_TEXTURE_2D, FBO.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, xava->w*resScale,
            xava->h*resScale, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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

    // bind SSBO - our shared GPU memory
    glGenBuffers    (1, &POST_SSBO);
    glBindBuffer    (GL_SHADER_STORAGE_BUFFER, POST_SSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, POST_SSBO);
    glBufferData    (GL_SHADER_STORAGE_BUFFER, sizeof(shaderMemory),
            shaderMemory, GL_DYNAMIC_COPY);
    glBindBuffer    (GL_SHADER_STORAGE_BUFFER, 0);

    glUseProgram(pre.program);

    if(gl_options.use_fft) {
        // reallocate and attach verticies data
        vertexData = realloc(vertexData, sizeof(GLfloat)*xava->bars*2);
        glVertexAttribPointer(PRE_BARS, 3, GL_FLOAT, GL_FALSE, 0, vertexData);
    } else {
        vertexData = realloc(vertexData, sizeof(GLfloat)*conf->inputsize*2);
        glVertexAttribPointer(PRE_AUDIO, 2, GL_FLOAT, GL_FALSE, 0, vertexData);
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
    SGLClear(xava);
}

XG_EVENT SGLEvent(struct XAVA_HANDLE *xava) {
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
void SGLClear(struct XAVA_HANDLE *xava) {
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

    glUseProgram(post.program);

    // set and attach foreground color
    glUniform4f(POST_FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // set background clear color
    glUniform4f(POST_BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

void SGLDraw(struct XAVA_HANDLE *xava) {
    struct config_params *conf = &xava->conf;
    struct audio_data    *audio = &xava->audio;

    // restrict time variable to one hour because floating point precision issues
    float currentTime = (float)fmodl((long double)xavaGetTime()/(long double)1000.0, 3600.0);
    float intensity = 0.0;

    /**
     * Here we start rendering to the texture
     **/

    if(gl_options.use_fft) {
        // i am speed
        for(register int i=0; i<xava->bars; i++) {
            float value = (float) xava->f[i] / xava->h;
            vertexData[i<<1] = i;
            vertexData[(i<<1)+1] = xava->f[i];
            intensity += value;
        }
    } else {
        for(register int i=0; i<conf->inputsize; i++) {
            vertexData[i<<1] = audio->audio_out_l[i];
            vertexData[(i<<1)+1] = conf->stereo ? audio->audio_out_r[i] : i;
        }
    }

    // enable blending temporary so that the colors get properly calculated on
    // the shader end of the pre stage
    if(gl_options.enable_blending) {
        glEnable(GL_BLEND);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_SRC_ALPHA, GL_ONE, GL_ZERO);
    }

    // since im not bothering to do the math, this'll do
    // - used to balance out intensity across various number of bars
    intensity /= xava->bars;

    // bind render target to texture
    glBindFramebuffer(GL_FRAMEBUFFER, FBO.framebuffer);
    glViewport(0, 0, xava->w*resScale, xava->h*resScale);

    // switch to pre shaders
    glUseProgram(pre.program);

    // update time
    glUniform1f(PRE_TIME, currentTime);

    // update intensity
    glUniform1f(PRE_INTENSITY, intensity);

    // clear the screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    GLuint vertexBuffer = 0;
    if(gl_options.use_fft)
        vertexBuffer = PRE_BARS;
    else
        vertexBuffer = PRE_AUDIO;

    // pointers get reset after each glUseProgram(), that's why this is done
    glVertexAttribPointer(vertexBuffer, 2, GL_FLOAT, GL_FALSE, 0, vertexData);

    glEnableVertexAttribArray(vertexBuffer);

    // You use the number of verticies, but not the actual polygon count
    if(gl_options.use_fft)
        glDrawArrays(GL_POINTS, 0, xava->bars);
    else
        glDrawArrays(GL_LINE_STRIP, 0, conf->inputsize);

    glDisableVertexAttribArray(vertexBuffer);

    // disable blending on the post stage as it produces
    // invalid colors on the window manager end
    glDisable(GL_BLEND);

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

    // bind GPU memory
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, POST_SSBO);
    glBufferData    (GL_SHADER_STORAGE_BUFFER, sizeof(shaderMemory),
            shaderMemory, GL_DYNAMIC_COPY);

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

    // disable shared GPU memory
    glBindBuffer    (GL_SHADER_STORAGE_BUFFER, 0);

}

void SGLDestroyProgram(struct SGLprogram *program) {
    glDeleteProgram(program->program);
}

void SGLCleanup(struct XAVA_HANDLE *xava) {
    // delete both pipelines
    SGLDestroyProgram(&pre);
    SGLDestroyProgram(&post);

    free(gradientColor);
    free(vertexData);
}

