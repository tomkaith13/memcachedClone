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

using namespace std;



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
    
    // server connection instance - main worker thread
    //void* serverInstance(void* arg);
    
    
public:
    
    // main constructor with default args
    MiniMemcached(string portNum = "11211",
                  int connectionCount = 10,
                  int cacheSize = 100,
                  string configFile = "") : mPortNum(portNum),
                                            mConnectionCount(connectionCount),
                                            mCacheSize(cacheSize),
                                            mConfigFile(configFile),
                                            mActiveConnCount(0) {};
    
    /*
     * Resource allocation done via initServer()
     * The server is read for use after this point.
     */
    bool initServer();
    
    /*
     * Resource deallocation via shutdown()
     */
    void shutdown();
    
    
};


#endif /* MiniMemcached_hpp */
