//
//  main.cpp
//  IPCTest
//
//  Created by apple on 2020/6/3.
//  Copyright © 2020 John Boiles . All rights reserved.
//

#include <iostream>
#define IPC_IMPLEMENTATION
#include <ipc.h>


int main(int argc, const char * argv[]) {
    std::cout << "42yeah is here" << std::endl;
    char name[] = "me.42yeah/shared";
    ipc_sharedmemory memory;
    ipc_mem_init(&memory, name, 1024);
    ipc_mem_create(&memory);
    std::cout << memory.name << std::endl;
    ipc_mem_close(&memory);
    return 0;
}
