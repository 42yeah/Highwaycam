//
//  frame.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "frame.hpp"
#include "app.hpp"
#include "program.hpp"


Frame::Frame(App *app, std::string name, std::string fpath) {
    init(app, name, fpath);
}

void Frame::renderToScreen(bool retina) {
    if (!retina) {
        glViewport(0, 0, size.x, size.y);
    } else {
        glViewport(0, 0, size.x * RETINA_MODIFIER, size.y * RETINA_MODIFIER);
    }
    float aspect = app->winSize.x / app->winSize.y;

    glUseProgram(program);
    if (prevPass != 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, prevPass);
        glUniform1i(uniform("prevPass"), 0);
        prevPass = 0;
    }
    glUniform1f(uniform("aspect"), aspect);
    glUniform1f(uniform("time"), app->time);
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void Frame::render() {
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glClear(GL_COLOR_BUFFER_BIT);

    renderToScreen(false);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint Frame::uniform(std::string name) {
    // Lazy initialize uniforms
    if (uniforms.count(name) <= 0) {
        uniforms[name] = glGetUniformLocation(program, name.c_str());
    }
    return uniforms[name];
}

void Frame::init(App *app, std::string name, std::string fpath) {
    this->app = app;
    this->prevPass = 0;
    this->name = name;
    program = link(app, "Shaders/vertex.glsl", fpath);
    
    float buf[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        -1.0f, 1.0f,
        -1.0f, -1.0f
    };
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    GLuint VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(buf), buf, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2.0f, nullptr);
    
    initTexture();
    size = app->winSize;
}

Frame &Frame::chain(Frame &frame) { 
    render();
    frame.prevPass = texture;
    return frame;
}

void Frame::initTexture() { 
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, app->winSize.x, app->winSize.y, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glGenFramebuffers(1, &FBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        app->warnings.push_back("Created framebuffer is incomlete complete.");
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Frame::destroyTexture() {
    glDeleteTextures(1, &texture);
    glDeleteFramebuffers(1, &FBO);
}

