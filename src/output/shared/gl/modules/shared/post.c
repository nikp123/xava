#include <string.h>

#include "post.h"
#include "util.h"
#include "../../../graphical.h"

// postVertices
static const GLfloat post_vertices[16] = {
    -1.0f, -1.0f,  // Position 0
     0.0f,  0.0f,  // TexCoord 0
     1.0f, -1.0f,  // Position 1
     1.0f,  0.0f,  // TexCoord 1
     1.0f,  1.0f,  // Position 2
     1.0f,  1.0f,  // TexCoord 2
    -1.0f,  1.0f,  // Position 3
     0.0f,  1.0f}; // TexCoord 3

void xava_gl_module_post_update_intensity(gl_module_post_render *vars) {
    // update intensity
    float intensity = xava_gl_module_util_calculate_intensity(vars->options->xava);
    glUniform1f(vars->gl_vars.INTENSITY, intensity);
}

void xava_gl_module_post_update_time(gl_module_post_render *vars) {
    float currentTime = xava_gl_module_util_obtain_time();

    // update time
    glUniform1f(vars->gl_vars.TIME, currentTime);
}

void xava_gl_module_post_update_colors(gl_module_post_render *vars) {
    float fgcol[4] = {
        ARGB_R_32(vars->options->xava->conf.col)/255.0,
        ARGB_G_32(vars->options->xava->conf.col)/255.0,
        ARGB_B_32(vars->options->xava->conf.col)/255.0,
        vars->options->xava->conf.foreground_opacity
    };

    float bgcol[4] = {
        ARGB_R_32(vars->options->xava->conf.bgcol)/255.0,
        ARGB_G_32(vars->options->xava->conf.bgcol)/255.0,
        ARGB_B_32(vars->options->xava->conf.bgcol)/255.0,
        vars->options->xava->conf.background_opacity
    };

    // set and attach foreground color
    glUniform4f(vars->gl_vars.FGCOL, fgcol[0], fgcol[1], fgcol[2], fgcol[3]);

    // set background clear color
    glUniform4f(vars->gl_vars.BGCOL, bgcol[0], bgcol[1], bgcol[2], bgcol[3]);
}

void xava_gl_module_post_config_load(gl_module_post_render *vars) {
    XAVAGLModuleOptions *options = vars->options;
    XAVA  *xava    = options->xava;
    xava_config_source           config = xava->default_config.config;

    char *shader;

    shader = xavaConfigGetString(config, "gl", "post_shader", "default");
    if(strcmp("none", shader)){
        xava_gl_module_shader_load(&vars->post, SGL_POST, SGL_VERT, shader, options);
        xava_gl_module_shader_load(&vars->post, SGL_POST, SGL_FRAG, shader, options);
        xava_gl_module_shader_load(&vars->post, SGL_POST, SGL_CONFIG, shader, options);
        vars->enabled = true;

        vars->features = 0;
        if(xavaConfigGetBool(vars->post.config, "features", "colors", false)) {
            vars->features |= GL_MODULE_POST_COLORS;
        }
        if(xavaConfigGetBool(vars->post.config, "features", "time", false)) {
            vars->features |= GL_MODULE_POST_TIME;
        }
        if(xavaConfigGetBool(vars->post.config, "features", "intensity", false)) {
            vars->features |= GL_MODULE_POST_INTENSITY;
        }
    } else {
        vars->enabled = false;
    }

}

void xava_gl_module_post_init(gl_module_post_render *vars) {
    if(!vars->enabled) return;

    struct gl_vars *gl = &vars->gl_vars;
    xava_gl_module_program *post = &vars->post;

    xava_gl_module_program_create(&vars->post);

    gl->POS        = glGetAttribLocation (post->program, "v_texCoord");
    gl->TEXCOORD   = glGetAttribLocation (post->program, "m_texCoord");
    gl->TEXTURE    = glGetUniformLocation(post->program, "color_texture");
    gl->DEPTH      = glGetUniformLocation(post->program, "depth_texture");
    gl->RESOLUTION = glGetUniformLocation(post->program, "resolution");

    gl->DOTS       = glGetAttribLocation (post->program, "v_dots");

    if(vars->features & GL_MODULE_POST_TIME)
        gl->TIME       = glGetUniformLocation(post->program, "time");
    if(vars->features & GL_MODULE_POST_INTENSITY)
        gl->INTENSITY  = glGetUniformLocation(post->program, "intensity");
    if(vars->features & GL_MODULE_POST_COLORS) {
        gl->FGCOL      = glGetUniformLocation(post->program, "foreground_color");
        gl->BGCOL      = glGetUniformLocation(post->program, "background_color");
    }
}

void xava_gl_module_post_apply(gl_module_post_render *vars) {
    if(!vars->enabled) return;

    XAVAGLModuleOptions *options = vars->options;
    XAVA *xava = options->xava;
    struct gl_vars *gl = &vars->gl_vars;

    glUseProgram(vars->post.program);

    // update screen resoltion
    glUniform2f(gl->RESOLUTION, xava->outer.w, xava->outer.h);

    // set texture properties
    glGenTextures(1,              &vars->FBO.final_texture);
    glBindTexture(GL_TEXTURE_2D,   vars->FBO.final_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
            xava->outer.w*options->resolution_scale,
            xava->outer.h*options->resolution_scale,
            0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set texture properties
    glGenTextures(1,             &vars->FBO.depth_texture);
    glBindTexture(GL_TEXTURE_2D,  vars->FBO.depth_texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            xava->outer.w*options->resolution_scale,
            xava->outer.h*options->resolution_scale,
            0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    // set framebuffer properties
    glGenFramebuffers(1,  &vars->FBO.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, vars->FBO.framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D, vars->FBO.final_texture, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
            GL_TEXTURE_2D, vars->FBO.depth_texture, 0);

    // check if it borked
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    xavaBailCondition(status != GL_FRAMEBUFFER_COMPLETE,
            "Failed to create framebuffer(s)! Error code 0x%X\n",
            status);
}

void xava_gl_module_post_pre_draw_setup(gl_module_post_render *vars) {
    XAVAGLModuleOptions *options = vars->options;
    XAVA  *xava    = options->xava;

    if(vars->enabled) {
        // bind render target to texture
        glBindFramebuffer(GL_FRAMEBUFFER, vars->FBO.framebuffer);
        glViewport(0, 0,
                xava->outer.w*options->resolution_scale,
                xava->outer.h*options->resolution_scale);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, xava->outer.w, xava->outer.h);
    }
}

void xava_gl_module_post_draw(gl_module_post_render *vars) {
    if(!vars->enabled) return;

    XAVAGLModuleOptions *options = vars->options;
    XAVA *xava = options->xava;

    /**
     * Once the texture has been conpleted, we now activate a seperate pipeline
     * which just displays that texture to the actual framebuffer for easier
     * shader writing
     * */
    // Change framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, xava->outer.w, xava->outer.h);

    // Switch to post shaders
    glUseProgram(vars->post.program);

    // update variables
    if(vars->features & GL_MODULE_POST_COLORS)
        xava_gl_module_post_update_colors(vars);
    if(vars->features & GL_MODULE_POST_TIME)
        xava_gl_module_post_update_time(vars);
    if(vars->features & GL_MODULE_POST_INTENSITY)
        xava_gl_module_post_update_intensity(vars);

    // Clear the color buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glPointSize(100.0);

    // set attribute array pointers for the texture unit and vertex unit
    glVertexAttribPointer(vars->gl_vars.POS, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat),  post_vertices);
    glVertexAttribPointer(vars->gl_vars.TEXCOORD, 2, GL_FLOAT,
        GL_FALSE, 4 * sizeof(GLfloat), &post_vertices[2]);

    // enable the use of the following attribute elements
    glEnableVertexAttribArray(vars->gl_vars.POS);
    glEnableVertexAttribArray(vars->gl_vars.TEXCOORD);

    // Bind the textures
    // Render texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, vars->FBO.final_texture);
    glUniform1i(vars->gl_vars.TEXTURE, 0);

    // Depth texture
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, vars->FBO.depth_texture);
    glUniform1i(vars->gl_vars.DEPTH, 1);

    // draw frame
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    glColor4f(1.0, 1.0, 1.0, 1.0);
    glDrawArrays(GL_POINTS, 0, 6);

    glDisableVertexAttribArray(vars->gl_vars.POS);
    glDisableVertexAttribArray(vars->gl_vars.TEXCOORD);
}

void xava_gl_module_post_cleanup(gl_module_post_render *vars) {
    if(vars->enabled) return;

    xava_gl_module_program_destroy(&vars->post);
}

