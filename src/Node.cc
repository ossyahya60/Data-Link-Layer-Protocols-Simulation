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

void Node::handleSendMsg(pair<string, string> msgPair, int seqNo, MyMessage_Base* rmsg)
{
    string errorBits = msgPair.first;
    string msgText = msgPair.second;
    double delay = 0;
    bool duplicated = false;
    //new msg for sending:
    MyMessage_Base* nmsg= new MyMessage_Base();
    nmsg->setMsgID(seqNo);
    nmsg->setM_Type(0); // 0 -> Data
    nmsg->setSendingTime(simTime().dbl());
    string newMsgText = "";
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);

    int no_of_itr = msgText.size();

    for(int i = -1; i < no_of_itr-1; i++){
        if(msgText[i + 1] == '$' || msgText[i + 1] == '/'){
            newMsgText += '/' ;
        }
        newMsgText += msgText[i + 1];
    }
    msgText = "$" + newMsgText + "$";
    cout <<"After Byte Stuffing:"<< msgText<<endl;

    std::bitset<8> generator_bits(generator);

    string errors = "";

    if(errorBits[0] =='1') //modification
    {
        errors += errors.length() > 0? ", modification" : "modification";
        int randChar = std::rand() % msgText.length(); // get random index of text message to error it
        msgText[randChar] += (std::rand() % 10)+1;
    }
    if(errorBits[1] =='1') //loss
    {
        errors += errors.length() > 0? ", loss" : "loss";
        //handle loss -> put on log file only:
        cout <<"Lost message: at time = "<< simTime().dbl() << " : " << msgText << " - " << seqNo << endl;
        int timeout = getParentModule()->par("timeout").intValue();
        MyMessage_Base* selfmsg= new MyMessage_Base();

        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        cout << nmsg->getNumberOfTransmissions() << endl;
        outStream << "- " << getName() << " timeout for message id=" << nmsg->getMsgID() << " at " << simTime() + timeout << endl;

        scheduleAt(simTime() + timeout, selfmsg);
        return;
    }
    if(errorBits[2] =='1') //duplication
    {
        errors += errors.length() > 0? ", duplication" : "duplication";
        duplicated = true;
    }
    if(errorBits[3] =='1') //delay
    {
        errors += errors.length() > 0? ", delay" : "delay";
        delay = getParentModule()->par("delay").doubleValue(); //get it from .ini file
    }

    nmsg->setM_Payload(msgText.c_str());
    if(delay>0){
        totalNumberOfTransmissions++;
        nmsg->setSendingTime(simTime().dbl()+ delay);
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        cout << nmsg->getNumberOfTransmissions() << endl;
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\"" << " at " << simTime() + delay << " with " << ((errors.length() > 0)? errors : "no errors") << endl;
        sendDelayed(nmsg, delay, "out");
    }
    else
    {
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        cout << nmsg->getNumberOfTransmissions() << endl;
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\"" << " at " << simTime() << " with " << ((errors.length() > 0)? errors : "no errors") << endl;
        send(nmsg, "out");
    }

    if (duplicated){
        MyMessage_Base* dupmsg = new MyMessage_Base();
        dupmsg->setMsgID(seqNo);
        dupmsg->setM_Type(0); // 0 -> Data
        dupmsg->setSendingTime(simTime().dbl()+delay+ 0.01);
        dupmsg->setM_Payload(msgText.c_str());
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        cout << nmsg->getNumberOfTransmissions() << endl;
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\"" << " at " << simTime() + delay + 0.01 << " with " << ((errors.length() > 0)? errors : "no errors") << endl;
        sendDelayed(dupmsg,delay+ 0.01, "out");
    }

    bitset<8> temp(msgText[0]);
    //cout<< (temp^generator_bits).to_string()<<endl;


}

void Node::handleRecieveMsg(int ackNo, MyMessage_Base* rmsg)
{
    MyMessage_Base* nmsg= new MyMessage_Base();
    nmsg->setM_Payload("ACK");
    nmsg->setPiggyBackingID(ackNo);
    nmsg->setM_Type(1); // 1 -> ACK
    nmsg->setSendingTime(simTime().dbl()+0.2);
    totalNumberOfTransmissions++;
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    cout << nmsg->getNumberOfTransmissions() << endl;
    outStream << "- " << getName() << " received message with id=" << rmsg->getMsgID() << " and content=\"" << rmsg->getM_Payload() << "\"" << " at " << simTime() << endl;
    sendDelayed(nmsg, 0.2, "out"); //TODO: handle delay time correctly
}

void Node::initialize()
{
    string nodeName = getName();
    nodeName = nodeName.substr(4, nodeName.length() - 4);
    // TODO - Generated method body
    string outputFileName = "../files/Outputs/pair";
    if(atoi(nodeName.c_str())%2==0){ //even index
        outputFileName += nodeName + to_string(atoi(nodeName.c_str()) + 1) + ".txt";
    } else { //odd
        outputFileName += to_string((atoi(nodeName.c_str()) - 1)) + nodeName + ".txt";
    }
    outStream.open("../files/Outputs/pair01.txt", std::fstream::in | std::fstream::out | std::fstream::app); //file name shouldn't be hard-coded
}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    if (msg->isSelfMessage()){
        handleSendMsg(dataMessages[Seq_Num], Seq_Num, check_and_cast<MyMessage_Base *> (msg));
        //then increment the seq. num. (msgID)
        Seq_Num++;
    }
    else{
       MyMessage_Base * rmsg = check_and_cast<MyMessage_Base *> (msg);
       int initTime = rmsg->getMsgID();
       if(rmsg->getM_Type() == -1 && initTime != -1) { //sender sent from coordinator
           MyMessage_Base* selfmsg= new MyMessage_Base();
           selfmsg->setM_Payload(rmsg->getM_Payload()); //TODO: read file and save it in a vector

           //process input file
           string fileName = rmsg->getM_Payload();
           fillSendData("../files/inputs/" + fileName);

           ///
           startTime = initTime - simTime().dbl();
           //

           isSender = true;
           scheduleAt(initTime - simTime(), selfmsg);
       }
       else if (rmsg->getM_Type() != -1){ //msg from node (a peer)

           if (isSender) {

               if(dataMessages.size() <= Seq_Num){
                   cout <<"At Sender: at time = "<< rmsg->getSendingTime() << " : "<< rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;
                   //TODO: signal the end of Node transmission

                   outStream << "- ..................." << endl;
                   outStream << "- end of input file" << endl;
                   outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                   outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                   outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                   outStream.close();

                   return;
               }

               if (Seq_Num == rmsg->getPiggyBackingID()){
                   //put that in the file
                   cout <<"At Sender: at time = "<< rmsg->getSendingTime() << " : "<< rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << endl;
                   handleSendMsg(dataMessages[Seq_Num], Seq_Num, rmsg);
                   //then increment the seq. num. (msgID)
                   Seq_Num++;
               }
               else { //discard this ACK!
                   cout <<"At Sender(discarded): at time = "<< rmsg->getSendingTime() << " : "<< rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << endl;
               }

           }
           else {
               string msg_recieved = rmsg->getM_Payload();
               string temp_byte_stuffing = "";
               int no_of_itr = msg_recieved.size();
               bool is_esc = false;
               for(int i = 1; i < no_of_itr-1; i++){
                   if(msg_recieved[i] != '/' ){
                       temp_byte_stuffing += msg_recieved[i] ;
                   }else if(msg_recieved[i] == '/' && is_esc){
                       temp_byte_stuffing += msg_recieved[i] ;
                       is_esc = false;
                   }
                   else{
                       is_esc = true;
                   }
               }
               cout <<"Removing the ByteStuffing: "<< temp_byte_stuffing<<endl;

               if(Ack_Num <= rmsg->getMsgID()) //less than for Lost case!
               {
                   Ack_Num = rmsg->getMsgID() + 1;
                   handleRecieveMsg(Ack_Num, rmsg);
                   //put that in file
                   cout <<"At Receiver: at time = "<< rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getMsgID() << endl;
               }
               else { //discard this frame (not proceeded to Network layer)
                   handleRecieveMsg(Ack_Num, rmsg);
                   cout <<"At Receiver(discarded) : at time = "<< rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getMsgID() << endl;
               }
           }
       }
    }
}
