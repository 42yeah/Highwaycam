//
//  app.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef app_hpp
#define app_hpp

#include <iostream>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "frame.hpp"


class App {
public:
    App(GLFWwindow *window);
    
    void update();
    
    void renderGUI();
    
    void mainLoop();
    
    void start();
    
    glm::vec2 winSize;
    GLFWwindow *window;
    std::vector<std::string> warnings;
    float time, lastInstant, deltaTime;
    
    Frame defaultFrame, invertFrame;
    
private:
    void configWindow(glm::vec2 size, glm::vec2 pos, bool inverse = false);
};

#endif /* app_hpp */
