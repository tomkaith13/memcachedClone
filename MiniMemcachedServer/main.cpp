//
//  main.cpp
//  MiniMemcachedServer
//
//

#include <iostream>
#include "MiniMemcached.hpp"

int main(int argc, const char * argv[]) {
    MiniMemcached m;
    
    if (m.initServer() == false) {
        cout<<" init failed"<<endl;
        return -1;
    }
    m.shutdown();
}
