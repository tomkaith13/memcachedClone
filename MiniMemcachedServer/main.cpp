//
//  main.cpp
//  MiniMemcachedServer
//
//  Created by BIBIN THOMAS on 2/25/17.
//  Copyright © 2017 tomkaith13. All rights reserved.
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
