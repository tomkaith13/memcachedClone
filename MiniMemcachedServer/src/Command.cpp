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

    
    
    if (mCmdString.empty())
        return CMD_INVALID;
    split(mCmdString, "\r\n", clientDataVec);
    vector<string> commandVec;
    split(clientDataVec[0], " ", commandVec);
    
    if (!commandVec.empty())
        commandTitle = commandVec[0];
    else {
        commandTitle = clientDataVec[0];
    }
    
    
    transform(commandTitle.begin(),
                   commandTitle.end(),
                   commandTitle.begin(),
                   ::tolower);
    
    if (commandTitle == "get") {
        if (validGetCommand(commandVec))
            return CMD_GET;
        else
            return CMD_INVALID;
    } else if (commandTitle == "set") {
        if (validSetCommand(commandVec))
            return CMD_SET;
        else
            return CMD_INVALID;
    } else if (commandTitle == "delete") {
        
        return CMD_DELETE;
    } else if (commandTitle == "quit") {
        
        return CMD_QUIT;
    }else
        return CMD_INVALID;
}

bool
Command::validSetCommand(vector<string> commandVec) {
    
    if (commandVec.size() < 5 || commandVec.size() > 6)
        return false;
    mCommand = commandVec[0];
    mKey = commandVec[1];
    mFlags = stoi(commandVec[2]);
    mExpTime = 0;
    mBytes = stoi(commandVec[4]);
    mNoReply = false;
    
    if (commandVec.size() == 6)
        mNoReply = true;
    
    return true;
}

bool
Command::validGetCommand(vector<string> commandVec) {
    
    if (commandVec.size() < 2)
        return false;
    
    mGetKeyVec = vector<string>(commandVec.begin()+1, commandVec.end());
    return true;
}
