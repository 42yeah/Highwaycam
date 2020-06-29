//
//  resource.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 27/06/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "resource.hpp"
#include <fstream>
#include <sstream>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include "app.hpp"


Resource::Resource(App *app, std::string name, std::string path) {
    init(app, name, path);
}

void Resource::init(App *app, std::string name, std::string path) {
    this->app = app;
    this->path = path;
    this->name = name;
    this->activated = false;
    this->type = UNKNOWN;
    this->width = 0;
    this->height = 0;
    this->imageFormat = GL_NONE;

    std::string imgSuffixList[] = { ".png", ".bmp", ".jpg", ".jpeg" };
    std::string videoSuffixList[] = { ".mp4" };
    for (std::string &suffix : imgSuffixList) {
        if (std::equal(suffix.rbegin(), suffix.rend(), name.rbegin())) {
            type = IMAGE;
            if (suffix == ".png") {
                this->imageFormat = GL_RGBA;
            } else {
                this->imageFormat = GL_RGB;
            }
            break;
        }
    }
    if (type == UNKNOWN) {
        for (std::string &suffix : videoSuffixList) {
            if (std::equal(suffix.rbegin(), suffix.rend(), name.rbegin())) {
                type = VIDEO;
                break;
            }
        }
    }
    if (type == UNKNOWN) {
        app->warnings.push_back("Unknown resource type: " + name);
        return;
    }
    switch (type) {
        case UNKNOWN:
            break;
            
        case IMAGE:
            loadTexture();
            break;
            
        case VIDEO:
            loadVideo();
            break;
    }
}

void Resource::loadTexture() { 
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    
    int channel;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &channel, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, imageFormat, GL_UNSIGNED_BYTE, data);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void Resource::loadVideo() { 
    // placeholder
}

