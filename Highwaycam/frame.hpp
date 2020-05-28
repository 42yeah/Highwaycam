//
//  frame.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef frame_hpp
#define frame_hpp

#include <map>
#include <glad/glad.h>
#include <glm/glm.hpp>

class App;


class Frame {
public:
    Frame() {}
    Frame(App *app, std::string fpath);
    
    void init(App *app, std::string fpath);
    
    void renderToScreen();
    void render();
    
    Frame &chain(Frame &frame);
    
    GLuint uniform(std::string name);

    GLuint VAO, program, FBO, texture;
    std::map<std::string, GLuint> uniforms;
    glm::vec2 size;
    GLuint prevPass;
    App *app;
};

#endif /* frame_hpp */
