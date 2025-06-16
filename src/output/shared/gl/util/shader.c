#include <stdio.h>
#include <string.h>

#include "output/shared/graphical.h"
#include "shader.h"
#include "shared.h"
#include "shared/ionotify.h"
#include "shared/log.h"

void xava_gl_module_shader_load(xava_gl_module_program *program,
                                sgl_shader_type type, sgl_shader_stage stage,
                                const char *name, XAVAGLModule *module,
                                XAVA *xava) {
    RawData *file;
    char *returned_path;
    char file_path[MAX_PATH];

    switch (type) {
        case SGL_PRE:
            strcpy(file_path, module->prefix);
            break;
        case SGL_POST:
            strcpy(file_path, "gl/shaders/");
            strcat(file_path, name);
            break;
        default:
            xavaBail("A really BIG oopsie happened here!");
            break;
    }

    struct shader *shader = NULL;
    switch (stage) {
        case SGL_VERT: {
            strcat(file_path, "/vertex.glsl");
            returned_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, file_path);
            xavaBailCondition(returned_path == NULL, "Failed to load '%s'!", file_path);
            shader = &program->vert;
            break;
        }
        case SGL_GEO: {
            xavaWarn("Avoid using geometry shaders as they are quite demanding");
            strcat(file_path, "/geometry.glsl");
            returned_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_OPTIONAL_CONFIG, file_path);
            xavaLogCondition(returned_path == NULL, "Failed to load '%s'!", file_path);
            shader = &program->geo;
            break;
        }
        case SGL_FRAG: {
            strcat(file_path, "/fragment.glsl");
            returned_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_CONFIG, file_path);
            xavaBailCondition(returned_path == NULL, "Failed to load '%s'!", file_path);
            shader = &program->frag;
            break;
        }
        case SGL_CONFIG: { // load shader config file
            strcat(file_path, "/config.ini");
            returned_path = xavaFindAndCheckFile(XAVA_FILE_TYPE_OPTIONAL_CONFIG, file_path);
            xavaLogCondition(returned_path == NULL, "Failed to load '%s'!", file_path);
            break;
        }
    }

    // abort if no file and mark as invalid
    if (returned_path == NULL) {
        switch (stage) {
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

    xava_ionotify_watch_setup setup = {0};

    // If no module is attached inotify reload should be ignored
    if (module != NULL) {
        // load file
        if (stage == SGL_CONFIG) {
            program->config = xavaConfigOpen(returned_path);

            // add watcher
            setup.filename = returned_path;
            setup.id = 1; // dont really care tbh
            setup.xava = xava;
            setup.ionotify = xava->ionotify;
            setup.xava_ionotify_func = module->func.ionotify_callback;
            xavaBailCondition(!xavaIONotifyAddWatch(setup),
                              "xavaIONotifyAddWatch failed!");
        } else {
            shader->path = strdup(returned_path);
            file = xavaReadFile(shader->path);
            shader->text = xavaDuplicateMemory(file->data, file->size);
            xavaCloseFile(file);

            // add watcher
            setup.filename = shader->path;
            setup.id = 1; // dont really care tbh
            setup.xava = xava;
            setup.ionotify = xava->ionotify;
            setup.xava_ionotify_func = module->func.ionotify_callback;
            xavaBailCondition(!xavaIONotifyAddWatch(setup),
                              "xavaIONotifyAddWatch failed!");
        }
    }

    // clean escape
    free(returned_path);
}

GLint xava_gl_module_shader_build(struct shader *shader, GLenum shader_type) {
    GLint status;

    shader->handle = glCreateShader(shader_type);
    xavaReturnErrorCondition(shader->handle == 0, 0, "Failed to build shader");

    glShaderSource(shader->handle, 1, (const char **)&shader->text, NULL);
    glCompileShader(shader->handle);

    glGetShaderiv(shader->handle, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1000] = {0};
        GLsizei len;
        glGetShaderInfoLog(shader->handle, 1000, &len, log);

        bool unknown_type = false;
        int file, line, char_num;
        size_t matched_elements = sscanf(log, "%d:%d(%d)", &file, &line, &char_num);
        if (matched_elements < 3) {
            size_t matched_elements = sscanf(log, "%d(%d)", &line, &char_num);
            if (matched_elements < 2)
                unknown_type = true;
        }

        if (unknown_type) {
            xavaError("Error: Compiling '%s' failed\n%*s", shader->path, len, log);
        } else {
            char *source_line;
            char *string_ptr = shader->text;
            for (int i = 0; i < line - 1; i++) {
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
    xava_gl_module_shader_build(&program->vert, GL_VERTEX_SHADER);
    if (program->geo
        .text) // optional stage, we check if it's included in the shader pack
        xava_gl_module_shader_build(&program->geo, GL_GEOMETRY_SHADER);
    xava_gl_module_shader_build(&program->frag, GL_FRAGMENT_SHADER);

    glAttachShader(program->program, program->vert.handle);
    if (program->geo
        .text) // optional stage, we check if it's included in the shader pack
        glAttachShader(program->program, program->geo.handle);
    glAttachShader(program->program, program->frag.handle);
    glLinkProgram(program->program);

    glDeleteShader(program->frag.handle);
    if (program->geo
        .text) // optional stage, we check if it's included in the shader pack
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

void xava_gl_module_program_destroy(xava_gl_module_program *program) {
    glDeleteProgram(program->program);
}
