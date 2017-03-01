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
    CMD_GETS,
    CMD_CAS,
    CMD_INVALID,
} ;

class Command {
    string mCmdString;
    vector<string> cmdString;
    string dataStr;
    
    // storage command members
    string mCommand;
    string mKey;
    int mFlags;
    int mExpTime;
    int mBytes;
    int mCasUnique;
    bool mNoReply;

    
    
    
    
    void split(const string& s, string delim, vector<string>& v);
public:
    Command(const char* cmd) : mCmdString(cmd) {};
    CommandType commandParse();
    
    //set command related functions
    bool validSetCommand(vector<string>);
    inline const string& getKeyFromSetCmd() { return mKey; }
    inline const int& getFlagsFromSetCmd() { return mFlags; }
    inline const int& getBytesFromSetCmd() { return mBytes; }
    inline const bool& getNoReplyFromSetCmd() { return mNoReply; }
    
    //get command related functions
    bool validGetCommand(vector<string>);
    vector<string> mGetKeyVec;
    inline const vector<string>& getKeysFromGetCmd() { return mGetKeyVec; }
    
    //delete command related functions
    bool validDeleteCommand(vector<string>);
    inline const string& getKeyFromDeleteCmd() { return mKey; }
    inline const bool& getNoReplyFromDeleteCmd() { return mNoReply; }
    
    //gets command related functions
    bool validGetsCommand(vector<string>);
    vector<string> mGetsKeyVec;
    inline const vector<string>& getKeysFromGetsCmd() { return mGetsKeyVec; }

    
    
};

#endif /* Command_hpp */
