//
//  server.cpp
//  Highwaycam
//
//  Created by Hao Zhou on 28/05/2020.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include "server.hpp"
#include "app.hpp"


Server::Server(App *app) : app(app) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
}

void Server::start() { 
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 16777343; // 127.0.0.1
    sin.sin_port = htons(PORT);
    
    int rc = bind(sock, (sockaddr *) &sin, sizeof(sin));
    if (rc < 0) {
        app->warnings.push_back("Failed to bind server at port " + std::to_string(PORT) + ". Port taken or insufficient memory?");
        return;
    }
    time.tv_sec = 0;
    time.tv_usec = 10;
}

void Server::respond() { 
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    int count = select(sock + 1, &set, nullptr, nullptr, &time);

    char expected[] = "frame";
    const int mtu = 1024;

    if (count > 0) {
        sockaddr_in sin;
        socklen_t slen = sizeof(sin);
        char data[512] = { 0 };
        ssize_t len = recvfrom(sock, data, sizeof(data), 0, (sockaddr *) &sin, &slen);
        if (len <= 0) { return; }
        
        // CMP && ACK
//        app->warnings.push_back("buffer: " + std::string(data) + " cmp: " + std::to_string(memcmp(expected, data, sizeof(expected) - 1)));
        if (memcmp(expected, data, sizeof(expected) - 1) == 0) {
            for (int i = 0; i < app->finalImageBuffer.first; i += mtu) {
                sendto(sock, app->finalImageBuffer.second + i, glm::min(app->finalImageBuffer.first - i, mtu), 0, (sockaddr *) &sin, slen);
                app->warnings.push_back("sending from " + std::to_string(i) + " to " + std::to_string(glm::min(app->finalImageBuffer.first - i, mtu)));
            }
        }
    }
}
