//
//  frame.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef frame_hpp
#define frame_hpp

#include <iostream>
#include <map>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

class App;


enum BindType {
    BIND_TEXTURE, BIND_FLOAT
};

class Frame {
public:
    Frame() {}
    Frame(App *app, std::string name, std::string fpath, int weight);
    
    void init(App *app, std::string name, std::string fpath, int weight);
    
    void renderToScreen(bool retina = true);
    void render();
    
    void initTexture();
    void destroyTexture();
    
    void bind(std::string uniformName, float value);
    void bind(std::string uniformName, GLuint value);
    
    Frame &chain(Frame &frame);
    
    GLuint uniform(std::string name);

    GLuint VAO, program, FBO, texture;
    std::map<std::string, GLuint> uniforms;
    
    std::vector<std::pair<std::string, GLuint> > extraTextures;
    
    glm::vec2 size;
    GLuint prevPass;
    App *app;
    std::string name;
    int weight;
};

#endif /* frame_hpp */
