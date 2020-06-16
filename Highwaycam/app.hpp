//
//  app.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef app_hpp
#define app_hpp

#define PORT 31256
#define RETINA_MODIFIER 2.0f

#include <iostream>
#include <vector>
#include <sstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "frame.hpp"
#include "server.hpp"
#include "camfetch.hpp"


class App {
public:
    App(GLFWwindow *window);
    
    void update();
    
    void renderGUI();
    
    void mainLoop();
    
    void start();
    
    void quit();
    
    void updateFinalImageBuffer();
    
    void updateCompressionStream();
    
    void updateFrames();
    
    std::pair<bool, Frame> readFrame(std::string path);
    
    // Borrowed from ImGui_Demo
    void helpMarker(std::string desc);
    
    glm::vec2 winSize;
    GLFWwindow *window;
    std::vector<std::string> warnings;
    float time, lastInstant, deltaTime;

    std::vector<std::pair<bool, Frame>> frames;
    Server server;
    std::pair<int, unsigned char *> finalImageBuffer;

    std::stringstream compressionStream;
    int compressQuality;
    int renderSendRatio;
    int numFramesRendered;
    bool horizontalFlip;

    Camera realCamera;

private:
    void configWindow(glm::vec2 size, glm::vec2 pos, bool inverse = false, bool collapsed = false);
};

#endif /* app_hpp */
