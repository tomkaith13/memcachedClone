//
//  Command.cpp
//  MiniMemcachedServer
//
//  Created by BIBIN THOMAS on 2/26/17.
//  Copyright © 2017 tomkaith13. All rights reserved.
//

#include "Command.hpp"


void
Command::split(const string& s, string delim, vector<string>& v) {
    unsigned long i = 0;
    auto pos = s.find(delim);
    while (pos != string::npos) {
        v.push_back(s.substr(i, pos-i));
        i = ++pos;
        pos = s.find(delim, pos);
        
        if (pos == string::npos)
            v.push_back(s.substr(i, s.length()));
    }
}
CommandType
Command::commandParse() {
    vector<string> clientDataVec;
    string commandTitle;

    split(mCmdString, "\r", clientDataVec);
    
    if (clientDataVec.empty())
        return CMD_INVALID;
    
    vector<string> commandVec;
    split(clientDataVec[0], " ", commandVec);
    
    if (commandVec.empty())
        commandTitle = clientDataVec[0];
    
    
    transform(commandTitle.begin(),
                   commandTitle.end(),
                   commandTitle.begin(),
                   ::tolower);
    
    if (commandTitle == "get") {
        
        return CMD_GET;
    } else if (commandTitle == "set") {
        
        return CMD_SET;
    } else if (commandTitle == "delete") {
        
        return CMD_DELETE;
    } else if (commandTitle == "quit") {
        
        return CMD_QUIT;
    } else
        return CMD_INVALID;
    
    
    
}
