//
//  Connection.cpp
//  MiniMemcachedServer
//
//

#include "Connection.hpp"


const int&
Connection::getClientSocket() {
    return mClientSoc;
}

void
Connection::start() {
    Job::start();
}
