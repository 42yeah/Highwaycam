//
//  server.hpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#ifndef server_hpp
#define server_hpp

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

class App;


class Server {
public:
    Server(App *app);
    
    void start();
    
    void respond();
    
    App *app;
    int sock;
    timeval time;
};

#endif /* server_hpp */
