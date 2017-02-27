//
//  Command.hpp
//  MiniMemcachedServer
//
//  Created by BIBIN THOMAS on 2/26/17.
//  Copyright © 2017 tomkaith13. All rights reserved.
//

#ifndef Command_hpp
#define Command_hpp

#include <stdio.h>
#include <string>
#include <vector>
#include <sstream>
#include <algorithm>

using namespace std;

enum CommandType{
    CMD_GET = 0,
    CMD_SET,
    CMD_DELETE,
    CMD_QUIT,
    CMD_INVALID,
} ;

class Command {
    string mCmdString;
    vector<string> cmdString;
    string dataStr;
    
    
    void split(const string& s, string delim, vector<string>& v);
public:
    Command(const char* cmd) : mCmdString(cmd) {};
    CommandType commandParse();
    
};

#endif /* Command_hpp */
