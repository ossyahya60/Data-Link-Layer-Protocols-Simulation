//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
//

#ifndef __SECTION1_NODE_H_
#define __SECTION1_NODE_H_

#include <omnetpp.h>
#include <bitset>
#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <algorithm>
#include "MyMessage_m.h"
using namespace std;
using namespace omnetpp;


/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
public:
    int Seq_Num = 0;
    int Ack_Num = 0;
    bool startNode = false;
    bool sentNACK = false;
    double startTime = 0;
    string generator = "11110111";
    vector<pair<string, string>> dataMessages;
    std::fstream outStream;
    int totalNumberOfTransmissions = 0;

    //for Selective Repeat protocol:
    int Sf = 0;// start of sending frame
    int Sn = 0;// sending frame number
    int Rn = 0;// receiving frame number
    int max_arrived_id = 0;
    vector<bool> arrived; //used to simulate the buffer for arrived frames
    vector<bool> sentArr;
    //helper functions:
    void fillSendData(string path);
    void handleData(pair<string, string>, int, MyMessage_Base *,double,bool);
    void handleACK(int, int, MyMessage_Base *);
    bitset<8> calculateCRC(string, bitset<8>);
    vector<int> generateHammingCode(vector<int> msgBits, int m, int r);
    vector<int> findHammingCode(vector<int>& msgBits);
    vector<int> generateMsg(vector<int> hamming, int m, int r);
    vector<int> findMsg(vector<int>& hamming);
    int receiveHamming(vector<int> hamming, bool& error);

protected:
    virtual void initialize();
    virtual void handleMessage(cMessage *msg);
};

#endif
