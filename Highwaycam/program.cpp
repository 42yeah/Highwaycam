//
//  program.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "program.hpp"
#include "app.hpp"
#include <sstream>


GLuint compile(App *app, GLuint type, std::string path) {
    std::string reason = "Failed to compile shader: " + path;
    GLuint shader = glCreateShader(type);
    std::fstream reader(path);
    if (!reader.good()) {
        app->warnings.push_back(reason + " (shader path not found)");
        return 0;
    }
    std::stringstream ss;
    ss << reader.rdbuf();
    std::string src = ss.str();
    const char *raw = src.c_str();
    glShaderSource(shader, 1, &raw, nullptr);
    glCompileShader(shader);
    char log[512] = { 0 };
    glGetShaderInfoLog(shader, sizeof(log), nullptr, log);
    if (std::string(log).length() > 0 && log[0] != '\n') {
        app->warnings.push_back(reason + " (" + log + ")");
    }
    return shader;
}

GLuint link(App *app, std::string vpath, std::string fpath) {
    std::string reason = "Failed to link program";
    GLuint program = glCreateProgram();
    glAttachShader(program, compile(app, GL_VERTEX_SHADER, vpath));
    glAttachShader(program, compile(app, GL_FRAGMENT_SHADER, fpath));
    glLinkProgram(program);
    char log[512] = { 0 };
    if (std::string(log).length() > 0 && log[0] != '\n') {
        app->warnings.push_back(reason + " (" + log + ")");
    }
    return program;
}

