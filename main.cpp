//
//  main.cpp
//  network proj 2
//
//  Created by 夏 夏 on 16/10/10.
//  Copyright © 2016年 Liangzhen Xia. All rights reserved.
//

#include <iostream>

#include "Header.h"




int main(int argc,  char * argv[]) {
    // insert code here...
    if (argc != 3) {
        perror("\n in the form of : ./server <serverIP> <serverPort> \n");
        exit(1);
    }
    serverIP = argv[1];
    serverPort = atoi(argv[2]);
    printf("\nserverIP:%s \nserverPort:%u \n", serverIP, serverPort);
    
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    
    int fd = newSocket(serverPort);
    char buf[BUFF];
    while (1) {
        bzero(buf, BUFF);
        int recvlength = recvfrom(fd, buf, BUFF, 0, (struct sockaddr *)& addr, &addrlen);
        if (recvlength > 4) {
            uint16_t temp = (buf[0] << 8) + buf[1];
            uint16_t opCode = temp        ;
            //uint16_t opCode = ntohs(temp);
            if (opCode == 1) {
                // RRQ
                client* newClient= new client(addr, &buf[2]);
                if (readRRQ(newClient))
                {   
                    cout << "label 1" << endl;
                    int thread;
                    pthread_t newThread;
                    if ((thread = pthread_create(&newThread, NULL, processRequest, (void *) newClient)) !=0) {
                        printf("\nError creating a thread:%d",thread);
                        exit(-1);
                    }
                }
                else
                    continue;
                
            }
        }
    }
    
    return 0;
}
