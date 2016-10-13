//
//  Header.h
//  
//
//  Created by Liangzhen Xia on 16/10/9.
//
//

#ifndef Header_h
#define Header_h

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <string.h>
#include <cmath>

#define BUFF 516


using namespace std;

char* serverIP;
uint16_t serverPort;

class client
{
public:
    struct sockaddr_in from;
    FILE* file;
    uint16_t socket;
    string filename;
    client(struct sockaddr_in ffrom, string ffilename){
        from = ffrom;
        filename = ffilename;
    }
};

int newSocket(uint16_t port = 0)
{
    int fd;
    if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("cannot create socket\n");
        return 0;
    }
    struct sockaddr_in address;
    memset((char*) &address, 0, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, serverIP, &(address.sin_addr));
    srand(time(NULL));
    if (port == 0) {
        int count = 0;
        while (count < 10) {
            count++;
            port = 2000 + rand()%(65535-2000);
            printf("port number: %u \n", port);
            address.sin_port = htons(port);
            if (::bind(fd, (struct sockaddr *)&address, sizeof(address))<0)
                continue;
            else
               return fd;
        }
        printf("\nfail to create a new socket\n");
        exit(2);
    }
    else
    {
        address.sin_port = htons(port);
        if (::bind(fd, (struct sockaddr *)&address, sizeof(address))<0)
        {
            printf("\nfail to bind the socket\n");
            exit(3);
        }
        
    }
    return fd;
}


char* createError(uint16_t errorCode, char* errorMsg)
{
    int length = strlen(errorMsg) + 5;
    char* buffer = new char[length];
    char* posi = buffer;
    bzero(buffer, length);
    uint16_t opCode = htons(3);
    memcpy(posi, &opCode, sizeof(opCode));
    posi += sizeof(opCode);
    uint16_t erCode = htons(errorCode);
    memcpy(posi, &erCode, 2);
    posi += 2;
    memcpy(posi, errorMsg, strlen(errorMsg));
    posi += strlen(errorMsg);
    uint8_t zero = 0;
    memcpy(posi, &zero, 1);

    // for(int i = 0; i < length; ++i){
    //     if(i < 4 || i == length - 1){
    //         cout << int(buffer[i]);
    //     }else
    //         cout << buffer[i];
    // }
    // cout << endl;

    return buffer;
  
}


char* createDATA(uint16_t blockNum, char* data)
{
    int length = strlen(data) +4;
    char* buffer = (char * )malloc(length);
    char* posi = buffer;
    bzero(posi, length);
    uint16_t opCode = htons(3);
    memcpy(posi, &opCode, 2);
    posi += 2;
    memcpy(posi, &blockNum, 2);
    posi += 2;
    memcpy(posi, data, strlen(data));
    return buffer;
}

bool readRRQ(client * newClient)
{
    const char* filename = newClient->filename.c_str();
    printf("RRQ for: %s \n", filename);
    FILE* fs = fopen(filename, "rb");
    cout << "FS: " << fs << endl;
    newClient->file = fs;
    if (fs == NULL) {
        printf("Can not find %s \n", filename);
        char errorMsg[] = "Can not find file.";
        char* errorbuffer = createError(1, errorMsg);
        uint16_t length = strlen(errorMsg) + 5;

        for(int i = 0; i < length; ++i){
            if(i < 4 || i == length - 1){
                cout << int(errorbuffer[i]);
            }else
                cout << errorbuffer[i];
        }
        cout << endl;



        cout << "error message length: " << length << endl;
        struct sockaddr *addr =(struct sockaddr *)(&(newClient->from));
        sendto(newClient->socket, errorbuffer, length, 0, addr, sizeof(struct sockaddr));
        delete(newClient);
        return false;
    }
    fseek(fs, 0L, SEEK_END);
    int size = ftell(fs);
    rewind(fs);
    if (size > (pow(2,16)-1)*512+511) {
        printf("the size of %s is larger thant the maximum size\n", filename);
        char errorMsg[] = "The file is too large to send.\n";
        char* errorbuffer = createError(3, errorMsg);
        uint16_t length = strlen(errorMsg) + 5;
        struct sockaddr * addr =(struct sockaddr *)&newClient->from;
        sendto(newClient->socket, errorbuffer, length, 0, addr, sizeof(addr));
        delete(newClient);
        return false;
    }
    newClient->socket = newSocket();
    return true;
}

void* processRequest(void *voidptr)
{
    
    client *ptr = ( client *) voidptr;

    struct sockaddr_in remoteaddr =  ptr-> from; /* remote address */
    socklen_t addrlen = sizeof(remoteaddr);
    
    int packetlen;
    char *pos;
    char *inputpos;
    char outputbuf[512+2+2];
    char inputbuf[2+2+512];		//Assuming error message cant be more than 512 bytes(including null)
    unsigned short int op, block;
    unsigned short int opcode, blockid = 1;
    
    size_t bytes;
    int flag = 0;
    int retransmitCount = 0;
    
    struct timeval tv;
    
    tv.tv_sec = 3;  /* 3 Secs Timeout */
    tv.tv_usec = 0;  // Not init'ing this can cause strange errors
    
    if (setsockopt(ptr->socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv, sizeof(struct timeval)) < 0)
    {
        printf("\nError in setting timeout value");
    }
    
    
    while(1)
    {
        if(flag == 0)	//send new data
        {
            cout << "sendint data..." << endl;
            //Read data from file
            cout << "filename: " << ptr->file << endl;
            FILE* aaa = fopen("a.txt", "rb");
            bytes = fread(&outputbuf[4], 1, 512, ptr->file);
            cout << "# of bytes read from file: " << bytes << endl;
            packetlen = sizeof( opcode ) + sizeof( blockid ) + bytes;
            //buf1 = (char *)malloc(packetlen);
            pos = outputbuf;
            
            //pack data in buffer
            op=htons(3);
            block=htons(blockid);
            memcpy( pos, &op, sizeof(op) );
            pos += sizeof(op);
            memcpy( pos, &block, sizeof(block) );
            pos += sizeof(block);
            printf("\nSent: %u \n", blockid);
            //cout << packetlen << endl;
            // for(int i = 0; i < packetlen; ++i){
            //     cout << outputbuf[i];
            // }
            // cout << endl;
            if(sendto(ptr->socket, outputbuf, packetlen, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0)
            {
                cout << "send error" << endl;
                fprintf(stderr, "ERROR: Failed to send file %s.\n", ptr->filename.c_str());
                flag = -1;
                break;
            }
            
            flag = 1;	  //indicates data send waiting for ack
            retransmitCount = 0;
            
            if(bytes < 512)
            {
                flag = 2; //indicates last packet
            }
            
        }
        
        //Read the client response
        if((recvfrom(ptr->socket, inputbuf, BUFF, 0, (struct sockaddr *)&remoteaddr, &addrlen)) >= (long)(2*sizeof(uint16_t)))	//Implies timeout
        {
            
            uint16_t recv_opcode = ntohs(*((uint16_t *)inputbuf));
            inputpos = inputbuf + sizeof(uint16_t);
            
            uint16_t recv_blockid;
            uint16_t errorcode;
            
            switch (recv_opcode)
            {
                    
                case 4:
                    recv_blockid = ntohs(*((uint16_t *)inputpos));
                    if(recv_blockid == blockid)
                    {
                        printf("\tACK %u",recv_blockid);
                        if(flag != 2)		//not last block
                        {
                            flag = 0;
                            ++blockid;
                        }
                    }
                    break;
                    
                case 5:
                    errorcode = ntohs(*((uint16_t *)inputpos));
                    inputpos += sizeof(uint16_t);
                    printf("\n\nError::");
                    switch (errorcode)
                {
                    case 1: printf("File not found.");
                        break;
                    case 2: printf("Access violation.");
                        break;
                    case 3: printf("Disk full or allocation exceeded.");
                        break;
                    case 4: printf("Illegal TFTP operation.");
                        break;
                    case 5: printf("Unknown transfer ID.");
                        break;
                    case 6: printf("File already exists.");
                        break;
                    case 7: printf("No such user");
                        break;
                    default:
                        break;
                }
                    printf(" %s",inputpos);
                    flag = -2;		//Indicates error message
                    break;
                    
                default:
                    break;
                    
            }
            
            if(flag != 0)	// implies last block or error condition
                break;
        }
        else
        {
            if(retransmitCount++ > 3)  //Retransmit three times 
                break;
            
            //Retransmitting
            if(sendto(ptr->socket, outputbuf, packetlen, 0, (struct sockaddr *)&remoteaddr, addrlen) < 0) 
            {
                fprintf(stderr, "ERROR: Failed to send file %s.\n", ptr->filename.c_str());
                break;
            }  		
            
        }
    }
    
    if(flag == 1)
        printf("\n File %s from server was Sent!\n", ptr->filename.c_str());
    close(ptr->socket);
    fclose(ptr->file);
    free(ptr);
    pthread_exit((void*) ptr);
}  



#endif /* Header_h */
