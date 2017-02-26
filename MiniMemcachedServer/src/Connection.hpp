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
    Connection(int sockFd, void* startFP (void*)) :
              mClientSoc(sockFd),
              Job(startFP,&mClientSoc) {};
    const int& getClientSocket();
    virtual void start();
};

#endif /* Connection_hpp */
