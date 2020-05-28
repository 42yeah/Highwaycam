//
//  program.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef program_hpp
#define program_hpp

#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class App;


GLuint compile(GLuint type, std::string path);

GLuint link(App *app, std::string vpath, std::string fpath);

#endif /* program_hpp */
