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
    sock = socket(AF_INET, SOCK_STREAM, 0);
}

void Server::start() { 
    sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = 16777343; // 127.0.0.1
    sin.sin_port = htons(PORT);
    
    int rc = bind(sock, (sockaddr *) &sin, sizeof(sin));
    listen(sock, 5);
    if (rc < 0) {
        app->warnings.push_back("Failed to bind server at port " + std::to_string(PORT) + ". Port taken or insufficient permission?");
        return;
    }
    time.tv_sec = 0;
    time.tv_usec = 10;
}

void Server::respond() { 
    fd_set set;
    FD_ZERO(&set);
    FD_SET(sock, &set);
    int highest = sock;
    for (int i = 0; i < connections.size(); i++) {
        FD_SET(connections[i], &set);
        if (connections[i] > highest) { highest = connections[i]; }
    }
    int count = select(highest + 1, &set, nullptr, nullptr, &time);

    if (count > 0) {
        if (FD_ISSET(sock, &set)) {
            sockaddr_in sin;
            socklen_t slen = sizeof(sin);
            int s = accept(sock, (sockaddr *) &sin, &slen);
            send(s, "syn", 3, 0);
            connections.push_back(s);
            app->warnings.push_back("Connection established");
        }
        for (int i = 0; i < connections.size(); i++) {
            if (FD_ISSET(connections[i], &set)) {
                char data[512] = { 0 };
                ssize_t len = recv(connections[i], data, sizeof(data), 0);
                if (len < 0) {
                    connections.erase(connections.begin() + i, connections.begin() + i + 1);
                    i--;
                    continue;
                }
                if (memcmp(data, "fetch", 5) == 0) {
                    app->updateCompressionStream();
                    std::string memory = app->compressionStream.str();
                    ssize_t size = memory.size();
                    app->warnings.push_back("Serving frame length: " + std::to_string(size));
                    send(connections[i], &size, sizeof(int), 0);
                    send(connections[i], memory.c_str(), size, 0);
                }
            }
        }
    }
}

// Just irresponsibly kill server socket
void Server::stop() {
    close(sock);
}
