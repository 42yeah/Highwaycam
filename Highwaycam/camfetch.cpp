//
//  CamFetch.cpp
//  IPFinal
//
//  Created by apple on 2020/1/8.
//  Copyright © 2020 aiofwa. All rights reserved.
//

#include "camfetch.hpp"
#include <glad/glad.h>


Camera::Camera(App *app, int id) : app(app), cameraTexture(0), ready(false) {
    cap.open(id + cv::CAP_ANY);
    cap.read(buffer);
    cap.set(3, getBufferWidth() / 2);
    cap.set(4, getBufferHeight() / 2);
    
    frame.init(app, "Default camera pass", "Shaders/camera.glsl", 10);
    ready = true;
}

cv::Mat &Camera::read() {
    if (!ready) { return buffer; }
    cap.read(buffer);
    if (!cameraTexture) {
        glGenTextures(1, &cameraTexture);
        glBindTexture(GL_TEXTURE_2D, cameraTexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getBufferWidth(), getBufferHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
        frame.bind("rawCamera", cameraTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, cameraTexture);
    }
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, getBufferWidth(), getBufferHeight(), 0, GL_BGR, GL_UNSIGNED_BYTE, getRawMemory());
    frame.render();
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return buffer;
}

void *Camera::getRawMemory() { 
    return &buffer.at<int>(0);
}

int Camera::getBufferWidth() {
    return buffer.cols;
}

int Camera::getBufferHeight() {
    return buffer.rows;
}
