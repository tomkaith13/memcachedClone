//
//  Connection.hpp
//  MiniMemcachedServer
//

#ifndef Connection_hpp
#define Connection_hpp

#include "Job.h"
#include "MiniMemcached.hpp"

class MiniMemcached;

class Connection : public Job {
    int mClientSoc;
    MiniMemcached* mServObj;
public:
    Connection() {};
    Connection(int sockFd, MiniMemcached* obj) :
              mClientSoc(sockFd), mServObj(obj) {};
    const int& getClientSocket();
    virtual void start();
};

#endif /* Connection_hpp */
