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

#include "Sender.h"

Define_Module(Sender);

void Sender::initialize()
{
   MyMessage_Base* msg= new MyMessage_Base();
   msg->setM_Payload("");
   msg->setM_Type(2);
   msg->setSeq_Num(Seq_Num++);
   scheduleAt(simTime() + 5, msg);
}

void Sender::handleMessage(cMessage *msg)
{
    // TODO
    //1) get the string from the user
    std::string input = "";
    std::cout << "Enter :" <<std::endl;
    std::cin >> input;
    //2) calculate the char count
    int inputCount = input.size();
    //3) define the vector of the bitsets
    std::vector<std::bitset<8>> vBits;
    //4) append the charcount to the vector
    std::cout << inputCount <<endl;
    vBits.push_back(std::bitset<8> (inputCount+2));
    //5) loop on the characters and append them to the vector
    //and calculate the paritty check simultaniouly
    std::bitset<8> parity (inputCount+2);

    for (int i=0; i<inputCount; i++){
        std::bitset<8> charc (input[i]);
        parity ^= charc;
        vBits.push_back(charc);
    }

    vBits.push_back(parity);
    //6) loop on the vector print the message and convert every bitset to char
    //and append them to the final string message
    // 7) do the modification to single bit
    std::string finalStr="";
    int randNum = std::rand() % 2;
    int randChar = (std::rand() % inputCount) +1;
    std::bitset<8> randBitset (randNum);
    std::cout << input << std::endl;
    int count = 0;
    for(auto& vElem : vBits){
        if(count==randChar){ //char to randomize it's first bit
            vElem ^= randBitset;
        }
        count++;
        std::cout << vElem.to_string() << endl;
        finalStr += (char)vElem.to_ulong();
    }

    MyMessage_Base * recieved_msg = check_and_cast<MyMessage_Base *> (msg);
    // should add the frame header payload, and the trailer to the string final
    MyMessage_Base * mmsg = new MyMessage_Base();
    if (recieved_msg->getM_Type() == 0){ //NACK
       mmsg->setM_Payload(finalStr.c_str());
       mmsg->setM_Type(2); // 2 represents Data
       mmsg->setSeq_Num(Seq_Num++); //don't increase seq number if NACK is received!
    }
    else{
       mmsg->setM_Payload(finalStr.c_str());
       mmsg->setM_Type(2); // 2 represents Data
       mmsg->setSeq_Num(Seq_Num++); //don't increase seq number if NACK is received!
    }

    cancelAndDelete(msg);

    std::cout << "Sending ..." <<endl;
    send(mmsg,"out");
}
