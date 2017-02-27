//
//  MiniMemcached.cpp
//  MiniMemcachedServer
//
//  Created  on 2/25/17.
//  Copyright © 2017. All rights reserved.
//

#include "MiniMemcached.hpp"

extern void* serverInstance(void*);

/*
 * Private api
 * This function creates and sets up socket for use.
 */
void MiniMemcached::connectionSetup() {
    
    int retVal;
    int yes = 1;
    
    memset(&mHints, 0, sizeof mHints);
    mHints.ai_family = AF_UNSPEC;
    mHints.ai_socktype = SOCK_STREAM;
    mHints.ai_flags = AI_PASSIVE; // find and use my IP
    
    if ((retVal = getaddrinfo(NULL, mPortNum.c_str(),
                              &mHints, &mServInfo)) != 0) {
        cerr<<"getaddrinfo:"<<gai_strerror(retVal)<<endl;
        throw "getaddrinfo failed";
    }
    
    struct addrinfo *iter;
    // loop through all the results and bind
    for(iter = mServInfo; iter != NULL;
        iter = iter->ai_next) {
        
        
        if ((mServSockFD = socket(iter->ai_family,
                             iter->ai_socktype,
                             iter->ai_protocol)) == -1) {
            perror("server: socket");
            continue;
        }
        
        if (setsockopt(mServSockFD, SOL_SOCKET, SO_REUSEADDR, &yes,
                       sizeof(int)) == -1) {
            throw "setsockopt failed";
        }
        
        if (::bind(mServSockFD, iter->ai_addr, iter->ai_addrlen) == -1) {
            close(mServSockFD);
            perror("server: bind");
            continue;
        }
        break;
    }//end for
    
    //no use for addrInfo struct once we create the server socket
    freeaddrinfo(mServInfo);
    
    if (iter == nullptr) {
        throw "server addrinfo structure is null";
    }
    
    if (listen(mServSockFD, 15) == -1) {
        perror("server: listen");
        throw "listen failed";
    }

    /*
    while(1) {
        cout<<"looping for accepting sockets"<<endl;
        socklen_t sin_size = sizeof mClientAddrInfo;
        mServIoFd = accept(mServSockFD, (struct sockaddr *)&mClientAddrInfo, &sin_size);
        if (mServIoFd == -1) {
            perror("server: accept");
            throw "accept failed";
        }
        
        char clientIp[INET6_ADDRSTRLEN];
        inet_ntop(mClientAddrInfo.ss_family,
                  get_in_addr((struct sockaddr *)&mClientAddrInfo),
                  clientIp, sizeof clientIp);
        
        //cout<<"Server received connection from client: "<<clientIp<<endl;
        printf("server for connection from client: %s\n", clientIp);
        
        
        // now we create threads to service the connections
        
        if (send(mServIoFd, "Hello, world!\n\n", 13, 0) == -1)
            perror("send");
        close(mServIoFd);
    }
     */
    
    
    // Creating a global thread pool to handle all connections
    try {
        mConnectionTPool = new ThreadPool(mConnectionCount);
    } catch(...) {
        throw "connectionSetup(): ThreadPool creation failed";
    }
    
    /* 
     * main thread just accepts new connections and adds to the jobQueue in the
     * thread pool. Each of these jobs are the ones who talk to the connected 
     * clients
     */
    while(1) {
        
        //if we exceeded the connection count then just wait
        {
            // lock before the cv access
            unique_lock<mutex> guard(mActiveConnMutex);
            
            //move ahead only if active connection are less than mConnCount
            mActiveConnCondV.wait(guard, [this]() -> bool {
                return mActiveConnCount < mConnectionCount;
            });
        }
        
        // if we reached here, that means we can still accept connections
        
        socklen_t sinSize = sizeof mClientAddrInfo;
        int clientIOSocketFd = accept(mServSockFD,
                                      (struct sockaddr *)&mClientAddrInfo,
                                      &sinSize);
        
        if (clientIOSocketFd == -1) {
            perror("server: accept");
            throw " connectionSetup(): accept failed";
        }
        
        { // scope for the mutex
            unique_lock<mutex> guard(mActiveConnMutex);
            if (mActiveConnCount < mConnectionCount) {
                mActiveConnCount++;
                mActiveConnSet.insert(clientIOSocketFd);
            } else {
                cerr<<"Active connections exceeded "<<mConnectionCount<<endl;
                close(clientIOSocketFd);
            }
        }
        
        mActiveConnCondV.notify_one();
        mConnectionTPool->AddJob(new Connection(clientIOSocketFd, this));
    }
}


void
MiniMemcached::serverInstance(int ioSocket) {
    
    string IntroStr;
    string command;
    
    cout<<"client socket:"<<ioSocket<<endl;
    cout<<"client socket set size:"<<mActiveConnSet.size()<<endl;
  
    IntroStr += "Client Connected\n\nReady to "
         "commands: SET, GET, DELETE and QUIT\n\n>";
    
    sendToClient(ioSocket, IntroStr);
    char commandBuff[MAX_BYTES_LIMIT];
    while(1) {
        memset(commandBuff, 0, MAX_BYTES_LIMIT);
        
        receiveFromClient(ioSocket, commandBuff);
        Command clientCmd(commandBuff);
        
        if (clientCmd.commandParse() == CMD_QUIT) {
            cout<<"quit is typed:"<<endl;
            string quitStr = "Connection Closed by Client\n";
            sendToClient(ioSocket, quitStr);            
            break;
        } else {
            //invalid command
            cout<<"invalid command: "<<commandBuff<<endl;
            sendToClient(ioSocket, "invalid command\n>");
            continue;
        }
        
        
    } //end while
    
    {
        unique_lock<mutex> guard(mActiveConnMutex);
        mActiveConnCount--;
        if (mActiveConnSet.find(ioSocket) != mActiveConnSet.end())
            mActiveConnSet.erase(mActiveConnSet.find(ioSocket));
        close(ioSocket);
    }
    mActiveConnCondV.notify_one();
    return;
}

void MiniMemcached::sendToClient(int sockfd, string message) {
    if (send(sockfd, message.c_str(), message.size(), 0) == -1) {
        perror("sending to client failed");
        throw "server instance failed to send message to client";
    }

}

void MiniMemcached::receiveFromClient(int sockfd, char* message) {
    
    if (recv(sockfd, message, MAX_BYTES_LIMIT, 0) == -1) {
        perror("receive from client failed");
        throw "server instance failed to receive message from client";
    }
}

void* MiniMemcached::get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
bool MiniMemcached::initServer() {
    try {
        connectionSetup();
    } catch(const char* s) {
        std::cout<<"Exception: "<<s<<endl;
        return false;
    }
    return true;
}
void MiniMemcached::shutdown() {
        close(mServSockFD);
        delete mConnectionTPool;
}
