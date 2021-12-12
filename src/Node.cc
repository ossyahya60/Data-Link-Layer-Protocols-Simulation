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

#include "Node.h"
#include "MyMessage_m.h"
Define_Module(Node);

void Node::fillSendData(string path)
{
    cout << path << endl;

    ifstream file(path);
    string line;
    //file >> line;
    while(getline(file,line)){
        pair<string, string> strPair(line.substr(0, 4), line.substr(5, line.length()));
        dataMessages.push_back(strPair); //TODO: handle Network error cases
    }

    file.close();
}

void Node::initialize()
{
    // TODO - Generated method body
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    if (msg->isSelfMessage()){
        MyMessage_Base* nmsg= new MyMessage_Base();
        nmsg->setM_Payload(dataMessages[Seq_Num].second.c_str());
        nmsg->setSeq_Num(Seq_Num++);
        nmsg->setM_Type(0); // 0 -> Data
        send(nmsg, "out");
    }
    else{
       MyMessage_Base * rmsg = check_and_cast<MyMessage_Base *> (msg);
       int startTime = rmsg->getSeq_Num();
       if(rmsg->getM_Type() == -1 && startTime != -1) { //sender sent from coordinator
           MyMessage_Base* selfmsg= new MyMessage_Base();
           selfmsg->setM_Payload(rmsg->getM_Payload()); //TODO: read file and save it in a vector

           //process input file
           string fileName = rmsg->getM_Payload();
           fillSendData("../files/inputs/" + fileName);

           isSender = true;
           scheduleAt(startTime - simTime(), selfmsg);
       }
       else if (rmsg->getM_Type() != -1){ //msg from node (a peer)
           cout << rmsg->getM_Payload() << " - " << rmsg->getSeq_Num() << endl;
           if (isSender) {
               MyMessage_Base* nmsg= new MyMessage_Base();

               if(dataMessages.size() <= Seq_Num)
                   //TODO: signal the end of Node transmission
                   return;

               nmsg->setM_Payload(dataMessages[Seq_Num].second.c_str());
               nmsg->setSeq_Num(Seq_Num++);
               nmsg->setM_Type(0); // 0 -> Data
               sendDelayed(nmsg, 2, "out"); //TODO: handle delay time correctly
           }
           else {
               MyMessage_Base* nmsg= new MyMessage_Base();
               nmsg->setM_Payload("ACK");
               nmsg->setSeq_Num(++Seq_Num);
               nmsg->setM_Type(1); // 1 -> ACK
               sendDelayed(nmsg, 2, "out"); //TODO: handle delay time correctly
           }
       }
    }

}



