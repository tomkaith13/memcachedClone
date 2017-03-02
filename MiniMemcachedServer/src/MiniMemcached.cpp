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
            } else {
                cerr<<"Active connections exceeded "<<mConnectionCount<<endl;
                close(clientIOSocketFd);
            }
        }
        
        mActiveConnCondV.notify_one();
        mConnectionTPool->AddJob(new Connection(clientIOSocketFd, this));
    }
}

/*
 * This is the function which runs in every worker thread
 * Unlike the main thread, this function is responsible for any client IO.
 */
void
MiniMemcached::serverInstance(int ioSocket) {
    
    string IntroStr;
    string command;
    
    cout<<"client socket:"<<ioSocket<<endl;
    cout<<"client socket set size:"<<mActiveConnCount<<endl;
  
    IntroStr += "Client Connected\n\nReady to "
         "commands: SET, GET, DELETE and QUIT\n\n>";
    
    sendToClient(ioSocket, IntroStr);
    char commandBuff[MAX_BYTES_LIMIT];
    while(1) {
        memset(commandBuff, 0, MAX_BYTES_LIMIT);
        
        receiveFromClient(ioSocket, commandBuff);
        Command clientCmd(commandBuff);
        
        //logic for set command
        if (clientCmd.commandParse() == CMD_SET) {
            cout<<"client issued set. Ready to accept data block"<<endl;
            
            memset(commandBuff, 0, MAX_BYTES_LIMIT);
            receiveFromClient(ioSocket, commandBuff);
            string val = string(commandBuff);
            
            struct memCachedVal mcCacheVal;
            
            mcCacheVal.mcBytes = clientCmd.getBytesFromSetCmd();
            mcCacheVal.mcCacheStrVal = val.substr(0, mcCacheVal.mcBytes);
            mcCacheVal.mcFlags = clientCmd.getFlagsFromSetCmd();
            mcCacheVal.mcExpTime = 0;
            
            /*
             * calculate and store the new hash from the original value
             * This is the hash that is returned by gets() and will be used
             * by cas to decided is the new value is stored or not.
             */
            
            mcCacheVal.mcCasVal = hash<string>{}(mcCacheVal.mcCacheStrVal);
            
            { // scope for the mutex to modify the unordered_map
                unique_lock<mutex> guard(mMemcachedMapMut);
                mMemcachedHashMap[clientCmd.getKeyFromSetCmd()] = mcCacheVal;
            }
            sendToClient(ioSocket, "STORED\n>");
            
            continue;
        } //logic for get command
        else if (clientCmd.commandParse() == CMD_GET) {
            vector<string> keyVec = clientCmd.getKeysFromGetCmd();
            
            
            for (auto key : keyVec) {
                struct memCachedVal hashVal;
                bool keyFound = false;
                
                {
                    unique_lock<mutex> guard(mMemcachedMapMut);
                    if (mMemcachedHashMap.find(key) !=
                        mMemcachedHashMap.end()) {
                            hashVal = mMemcachedHashMap[key];
                            keyFound = true;
                    }
                }
                string output;
                
                if (keyFound) {
                    output.append("VALUE ");
                    output.append(key);
                    output.append(" ");
                    output.append(to_string(hashVal.mcFlags));
                    output.append(" ");
                    output.append(to_string(hashVal.mcBytes));
                    output.append("\n");
                    output.append(hashVal.mcCacheStrVal);
                    output.append("\n");
                    
                    sendToClient(ioSocket, output);
                    output.clear();
                }
                
            } //end for
            sendToClient(ioSocket, "END\n>");
            continue;
        } // If the user tries to delete a key
        else if (clientCmd.commandParse() == CMD_DELETE) {
            string deleteKey = clientCmd.getKeyFromDeleteCmd();
            bool deletedFlg = false;
            
            { //mutex scope
                unique_lock<mutex> guard(mMemcachedMapMut);
                if (mMemcachedHashMap.find(deleteKey) !=
                    mMemcachedHashMap.end()) {
                    mMemcachedHashMap.erase(deleteKey);
                    deletedFlg = true;
                }
            }
            
            if (deletedFlg)
                sendToClient(ioSocket, "DELETED\r\n>");
            else
                sendToClient(ioSocket, "NOT_FOUND\r\n>");
            
            continue;
            
        } // If the user tries to get a key with cas_unique using gets()
        else if (clientCmd.commandParse() == CMD_GETS) {
            
            vector<string> keyVec = clientCmd.getKeysFromGetsCmd();
            
            for (auto key : keyVec) {
                struct memCachedVal hashVal;
                bool keyFound = false;
                
                {
                    unique_lock<mutex> guard(mMemcachedMapMut);
                    if (mMemcachedHashMap.find(key) != mMemcachedHashMap.end()) {
                        hashVal = mMemcachedHashMap[key];
                        keyFound = true;
                    }
                    
                    
                }
                if (keyFound) {
                    string output;
                    output.append("VALUE ");
                    output.append(key);
                    output.append(" ");
                    output.append(to_string(hashVal.mcFlags));
                    output.append(" ");
                    output.append(to_string(hashVal.mcBytes));
                    output.append(" ");
                    output.append(to_string(hashVal.mcCasVal));
                    output.append("\n");
                    output.append(hashVal.mcCacheStrVal);
                    output.append("\n");
                    
                    sendToClient(ioSocket, output);
                    output.clear();
                }
                
            }
            sendToClient(ioSocket, "END\n>");
            continue;
            
        } else if (clientCmd.commandParse() == CMD_CAS) {
            memset(commandBuff, 0, MAX_BYTES_LIMIT);
            receiveFromClient(ioSocket, commandBuff);
            string val = string(commandBuff);
            
            struct memCachedVal mcCacheVal;
            
            mcCacheVal.mcBytes = clientCmd.getBytesFromCasCmd();
            mcCacheVal.mcCacheStrVal = val.substr(0, mcCacheVal.mcBytes);
            mcCacheVal.mcFlags = clientCmd.getFlagsFromCasCmd();
            mcCacheVal.mcExpTime = 0;
            mcCacheVal.mcCasVal = hash<string>{}(mcCacheVal.mcCacheStrVal);
            //mcCacheVal.mcCasVal = clientCmd.getCasUniqueFromCasCmd();
            string casCmdKey = clientCmd.getKeyFromCasCmd();
            string output;
            
            { // scope of the mutex
                unique_lock<mutex> guard(mMemcachedMapMut);
                if (mMemcachedHashMap.find(casCmdKey) == mMemcachedHashMap.end()) {
                    output =  "NOT_FOUND\r\n>";
                    continue;
                } else {
                    /* 
                     * only if the cas_unique val given in the cas command
                     * is same as the one already stored in the hash, we
                     * change the value to the new value
                     */
                    
                    if (clientCmd.getCasUniqueFromCasCmd() ==
                        mMemcachedHashMap[casCmdKey].mcCasVal) {
                        mMemcachedHashMap[casCmdKey] = mcCacheVal;
                        output = "STORED\r\n>";
                    } else {
                        output = "EXISTS\r\n>";
                    }
                }
            }
            
            sendToClient(ioSocket, output);
            continue;
            
            
        } // if user types quit
        else if (clientCmd.commandParse() == CMD_QUIT) {
            cout<<"quit is typed:"<<endl;
            string quitStr = "Connection Closed by Client\n";
            sendToClient(ioSocket, quitStr);            
            break;
        } // any other invalid command is greeted with ERROR
        else {
            //invalid command
            cout<<"invalid command: "<<commandBuff<<endl;
            sendToClient(ioSocket, "ERROR\r\n>");
            continue;
        }
        
        
    } //end while
    
    {
        unique_lock<mutex> guard(mActiveConnMutex);
        mActiveConnCount--;
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
