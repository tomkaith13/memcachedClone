//
//  MiniMemcached.hpp
//  MiniMemcachedServer
//
//  Created  on 2/25/17.
//  Copyright © 2017. All rights reserved.
//

#ifndef MiniMemcached_hpp
#define MiniMemcached_hpp

#include <iostream>
#include <unordered_set>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "ThreadPool.hpp"
#include "Connection.hpp"
#include "Command.hpp"

using namespace std;

// LIMITS MACRO
#define MAX_BYTES_LIMIT 100
#define MAX_CONNECTIONS_LIMIT 10
#define MAX_CACHE_SIZE 100
#define DEFAULT_PORT "11211"


/*
 * This is the main class for mini-memcached which accepts resource 
 * configurables via arguments.
 */
class MiniMemcached {
    // port number for the server
    string mPortNum;
    
    // total number of accepted connection
    int mConnectionCount;
    
    // cache size for the key value pair evicted via LRU once full.
    int mCacheSize;
    
    // config file path
    string mConfigFile;
    
    //server socket fd - for accepting new connections
    int mServSockFD;
    
    //max limit of bytes sent/received
    int mMaxBytesLimit;
    
    /*
     * unordered_set of client connection
     */
    mutex mActiveConnMutex;
    condition_variable mActiveConnCondV;
    int mActiveConnCount;
    unordered_set<int> mActiveConnSet;
    
    struct sockaddr_storage mClientAddrInfo;
    
    //socket related structures
    struct addrinfo mHints, *mServInfo;
    
    //connections threadpool
    ThreadPool* mConnectionTPool;
    
    /*
     *  private apis
     */
    
    void connectionSetup();
    void* get_in_addr(struct sockaddr *);
    
    void sendToClient(int sockFD, string);
    void receiveFromClient(int sockFD, char* );
    
    
    
public:
    
    // main constructor with default args
    MiniMemcached(string portNum = DEFAULT_PORT,
                  int connectionCount = MAX_CONNECTIONS_LIMIT,
                  int cacheSize = MAX_CACHE_SIZE,
                  string configFile = "") : mPortNum(portNum),
                                            mConnectionCount(connectionCount),
                                            mCacheSize(cacheSize),
                                            mConfigFile(configFile),
                                            mMaxBytesLimit(MAX_BYTES_LIMIT),
                                            mActiveConnCount(0) {};
    
    /*
     * Resource allocation done via initServer()
     * The server is read for use after this point.
     */
    bool initServer();
    
    // server connection instance - main worker thread
    void serverInstance(int);
    
    /*
     * Resource deallocation via shutdown()
     */
    void shutdown();
    
    
};


#endif /* MiniMemcached_hpp */
