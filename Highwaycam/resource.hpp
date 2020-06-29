//
//  resource.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 27/06/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef resource_hpp
#define resource_hpp

#include <iostream>
#include <glad/glad.h>


class App;

enum ResourceType {
    UNKNOWN, VIDEO, IMAGE
};

class Resource {
public:
    Resource() {}
    Resource(App *app, std::string name, std::string path);
    
    void init(App *app, std::string name, std::string path);
    
    void loadTexture();
    void loadVideo();
    
    std::string path;
    std::string name;
    bool activated;
    ResourceType type;
    GLuint texture;
    void *video; // placeholder
    App *app;
    GLuint imageFormat;
    
    int width;
    int height;
};

#endif /* resource_hpp */
