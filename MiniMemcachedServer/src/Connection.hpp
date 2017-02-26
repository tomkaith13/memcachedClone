//
//  Connection.hpp
//  MiniMemcachedServer
//

#ifndef Connection_hpp
#define Connection_hpp

#include "Job.h"

class Connection : public Job {
    int mClientSoc;
public:
    Connection(int sockFd, void* startFP (void*), void* arg) :
              mClientSoc(sockFd),
              Job(startFP,&mClientSoc) {};
    const int& getClientSocket();
    virtual void start();
};

#endif /* Connection_hpp */
