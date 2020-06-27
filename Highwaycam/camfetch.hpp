//
//  CamFetch.hpp
//  IPFinal
//
//  Created by apple on 2020/1/8.
//  Copyright © 2020 aiofwa. All rights reserved.
//

#ifndef CamFetch_hpp
#define CamFetch_hpp

#include <opencv2/opencv.hpp>
#include "frame.hpp"


class Camera {
public:
    Camera() : ready(false) {}
    Camera(App *app, int id);
    
    cv::Mat &read();
    void *getRawMemory();
    int getBufferWidth();
    int getBufferHeight();
    
    GLuint cameraTexture;
    App *app;
    Frame frame;
    bool ready;
    bool paused;

private:
    cv::VideoCapture cap;
    cv::Mat buffer;
};

#endif /* CamFetch_hpp */
