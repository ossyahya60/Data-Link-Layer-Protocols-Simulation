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

#include "Receiver.h"

Define_Module(Receiver);

void Receiver::initialize()
{
    // TODO - Generated method body
}

void Receiver::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    MyMessage_Base * mmsg = check_and_cast<MyMessage_Base *> (msg);
    std::string msgStr = mmsg->getM_Payload();
    int msgSize = msgStr.size();
    if(msgSize != 0){
        std::bitset<8> parity(0);
        std::bitset<8> incomingParity(msgStr[msgSize-1]);
        std::string message = "";
        for (int i=0; i<msgSize-1; i++){
          std::bitset<8> charc (msgStr[i]);
          parity ^= charc;
          if(i!=0){
              message += msgStr[i];
          }
        }

        if((parity ^ incomingParity).to_ulong() == 0){
          //nice
           mmsg->setM_Type(1); // 1 represents ACK
           //std::string str = mmsg->getM_Payload();
           std::cout << "Message Received Successfully - "<< message <<endl;
        }
        else {
           mmsg->setM_Type(0); // 0 represents NACK
           std::cout << "Message was Corrupted - "<< message  <<endl;
        }
    }
    //cMessage* cmsg = new cMessage("Message");
    send(mmsg,"out");
}
