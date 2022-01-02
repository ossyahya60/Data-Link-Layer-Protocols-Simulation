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
#define propagationDelay 0.2

void Node::fillSendData(string path)
{
    cout << path << endl;

    ifstream file(path);
    string line;
    //file >> line;
    while (getline(file, line))
    {
        pair<string, string> strPair(line.substr(0, 4), line.substr(5, line.length()));
        dataMessages.push_back(strPair); //TODO: handle Network error cases
    }

    file.close();
}

// This function calculates the CRC given the msg after being converted to bits and the generator
// and returns the reminder of their division
bits Node::calculateCRC(string msg_in_bits, bitset<8> generator_function)
{
    int size = msg_in_bits.size() - 8;
    string rem = msg_in_bits.substr(0, 7);

    // In each iteration, the rem window is shifted by one bit and then is divided by the generator
    for (int i = 0; i <= size; i++)
    {
        if (i != 0)
        {
            rem = rem.substr(1) + msg_in_bits[i + 7];
        }
        if (rem[0] == '1')
        {
            bitset<8> temp_rem(rem);
            rem = (temp_rem ^ generator_function).to_string();
        }
    }
    bitset<8> rem_in_bits(rem);
    return rem_in_bits;
}

void Node::handleData(pair<string, string> msgPair, int seqNo, MyMessage_Base *rmsg, double betweenFramesDelay)
{
    string errorBits = msgPair.first;
    string msgText = msgPair.second;
    double delay = 0;
    bool duplicated = false;
    bool lose = false;
    //new msg for sending:
    MyMessage_Base *nmsg = new MyMessage_Base();
    nmsg->setMsgID(seqNo);
    nmsg->setPiggyBackingID(rmsg->getPiggyBackingID());
    nmsg->setM_Type(0); // 0 -> Data
    string newMsgText = "";
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    string msg_in_bits = "";

    // Byte stuffing with start and end flag + Converting the string into bits stream
    int no_of_itr = msgText.size();
    bitset<8> start_byte('$');
    msg_in_bits += start_byte.to_string();
    for (int i = -1; i < no_of_itr - 1; i++)
    {
        if (msgText[i + 1] == '$' || msgText[i + 1] == '/')
        {
            newMsgText += '/';
            bitset<8> temp('/');
            msg_in_bits += temp.to_string();
        }
        bitset<8> temp(msgText[i + 1]);
        msg_in_bits += temp.to_string();
        newMsgText += msgText[i + 1];
    }
    msg_in_bits += start_byte.to_string();

    msgText = "$" + newMsgText + "$";
    msg_in_bits += "0000000";

    //Set the CRC of the msg
    bitset<8> generator_bits(generator);
    bitset<8> crc = calculateCRC(msg_in_bits, generator_bits);
    nmsg->setCrc(crc);

    string errors = "";

    if (errorBits[0] == '1') //modification
    {
        errors += errors.length() > 0 ? ", modification" : "modification";
        int randChar = std::rand() % msgText.length(); // get random index of text message to error it
        msgText[randChar] += (std::rand() % 10) + 1;
    }
    if (errorBits[1] == '1') //loss
    {
        errors += errors.length() > 0 ? ", loss" : "loss";
        loss =true;
    }
    if (errorBits[2] == '1') //duplication
    {
        errors += errors.length() > 0 ? ", duplication" : "duplication";
        duplicated = true;
    }
    if (errorBits[3] == '1') //delay
    {
        errors += errors.length() > 0 ? ", delay" : "delay";
        delay = getParentModule()->par("delay").doubleValue(); //get it from .ini file
    }

    nmsg->setM_Payload(msgText.c_str());

    totalNumberOfTransmissions++;

    if(loss){
        //MyMessage_Base *selfmsg = new MyMessage_Base();
        //selfmsg->setM_Payload("Lost!");
        //selfmsg->setSendingTime(simTime().dbl() + delay + betweenFramesDelay + propagationDelay);
        //selfmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " timeout for message id=" << nmsg->getMsgID() << " at " << simTime() + delay + betweenFramesDelay + propagationDelay << endl;
        cout << getName() << " lost for message id=" << nmsg->getMsgID() << " at " << simTime() + delay + betweenFramesDelay + propagationDelay << endl;
        //scheduleAt(simTime().dbl() + delay + betweenFramesDelay + propagationDelay, selfmsg);


        //as if we send the next frame - and this frame is lost:
        /*Seq_Num++;
        nmsg->setMsgID(Seq_Num);
        msgText = dataMessages[Seq_Num];
        seqNo=Seq_Num;
        delay+=0.05;*/
    }
    nmsg->setSendingTime(simTime().dbl() + delay + betweenFramesDelay + propagationDelay);
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
              << " at " << simTime() + delay + betweenFramesDelay + propagationDelay << " with " << ((errors.length() > 0) ? errors : "no errors") << endl;
    sendDelayed(nmsg, propagationDelay+delay +betweenFramesDelay, "out");


    if (duplicated) /*&& !lost)*/ //8aleban hanfkes ll duplication if msg was lost
    {
        MyMessage_Base *dupmsg = new MyMessage_Base();
        dupmsg->setMsgID(seqNo);
        dupmsg->setM_Type(0); // 0 -> Data
        dupmsg->setSendingTime(simTime().dbl() + delay + 0.01 + betweenFramesDelay + propagationDelay);
        dupmsg->setM_Payload(msgText.c_str());
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
                  << " at " << simTime() + delay + betweenFramesDelay + propagationDelay + 0.01 << " with " << ((errors.length() > 0) ? errors : "no errors") << endl;
        sendDelayed(dupmsg, propagationDelay + delay + 0.01 + betweenFramesDelay, "out");
    }
}

void Node::handleACK(int ackNo, int mtype, MyMessage_Base *rmsg)
{
    rmsg->setPiggyBackingID(ackNo);
    //cout << "Piggypack" <<endl
    rmsg->setM_Type(mtype); // 1 -> ACK , 2 -> NACK
    rmsg->setSendingTime(simTime().dbl() + propagationDelay);
    totalNumberOfTransmissions++;
    rmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    outStream << "- " << getName() << " received message with id=" << rmsg->getMsgID() << " and content=\"" << rmsg->getM_Payload() << "\""
              << " at " << simTime() << endl;
    //sendDelayed(nmsg, propagationDelay, "out");
}

void Node::initialize()
{
    //initialize arrived vector:
    int windowSize = getParentModule()->par("windowSize").intValue();
    for(int i=0; i<windowSize; ++i){
        arrived.push_back(false);
    }
}

void Node::handleMessage(cMessage *msg)
{
    int windowSize = getParentModule()->par("windowSize").intValue();
    // TODO - Generated method body
    if (msg->isSelfMessage())
    {
        cout << getName() << " will start sending:" <<endl;
        MyMessage_Base* rmsg = check_and_cast<MyMessage_Base *>(msg);
        Ack_Num = rmsg->getPiggyBackingID();
        Rn = Ack_Num;
        handleACK(Ack_Num, 1, rmsg);
        if (dataMessages.size() <= Seq_Num)
        {
            string str = rmsg->getM_Payload();
            if(str == "Finished")//other node send "Finish" as a terminating flag (both now finished)
            {
                cout << "We are DONE!!" <<endl;
                endSimulation();
            }

            totalNumberOfTransmissions++;
            rmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
            rmsg->setM_Payload("Finish");
            sendDelayed(rmsg,propagationDelay, "out");
            return;
        }
        for(int i=0; i<windowSize; ++i){ //only for first time:

            handleData(dataMessages[Seq_Num], Seq_Num, rmsg,0.05*i);
            //then increment the seq. num. (msgID)
            Seq_Num++;
            Sf = Ack_Num; //start of frame
            Sn = Seq_Num;

            //printing:
            cout <<getName() << "    <Send Part> -sent frame: "<< to_string(Sn-1)
                 << " Sf=" <<to_string(Sf) << " Sn=" <<to_string(Sn) << " sent ACK: " << rmsg->getPiggyBackingID()
                 << "    <Recv Part> Rn=" <<to_string(Rn)<< " At "<< simTime()+0.05*i <<endl;
        }
    }
    else
    {
        MyMessage_Base *rmsg = check_and_cast<MyMessage_Base *>(msg);
        //come from coordinator:
        if (rmsg->getM_Type() == -1){
            string nodeName = getName();
            nodeName = nodeName.substr(4, nodeName.length() - 4);
            string outputFileName = "../files/Outputs/pair";
            if (atoi(nodeName.c_str()) % 2 == 0)
            { //even index
                outputFileName += nodeName + to_string(atoi(nodeName.c_str()) + 1) + ".txt";
            }
            else
            { //odd
                outputFileName += to_string((atoi(nodeName.c_str()) - 1)) + nodeName + ".txt";
            }
            outStream.open(outputFileName, std::fstream::in | std::fstream::out | std::fstream::app); //file name shouldn't be hard-coded

            //process input file
            string fileName = rmsg->getM_Payload();
            fillSendData("../files/inputs/" + fileName);
            //
        }

        int initTime = rmsg->getMsgID();
        //msg sent from coordinator (start node)
        if (rmsg->getM_Type() == -1 && initTime != -1)
        {
            startTime = initTime - simTime().dbl();
            startNode = true;
            MyMessage_Base *selfmsg = check_and_cast<MyMessage_Base *>(msg);
            selfmsg->setPiggyBackingID(0);//at the start: ACK = 0
            selfmsg->setMsgID(0);//at the start
            scheduleAt(initTime - simTime(), selfmsg);
        }
        else if (rmsg->getM_Type() != -1)
        {

            //msg from node (a peer)
            MyMessage_Base *newmsg = new MyMessage_Base();
            newmsg->setPiggyBackingID(rmsg->getPiggyBackingID());
            newmsg->setMsgID(rmsg->getMsgID());
            newmsg->setM_Payload(rmsg->getM_Payload());
            newmsg->setM_Type(rmsg->getM_Type());

            ////////////////////////////////////////
            ///     Receive Data -> Send ACK     ///
            ////////////////////////////////////////

            string msg_recieved = rmsg->getM_Payload();
            string temp_byte_stuffing = "";
            string msg_in_bits = "";

            bitset<8> generator_bits(generator);
            int no_of_itr = msg_recieved.size();
            // a boolean that is set if the previous character was an esc
            // to prevent the esc of every esc in the string
            bool is_esc = false;
            bitset<8> start_byte(msg_recieved[0]);
            msg_in_bits += start_byte.to_string();
            // Remove the byte stuffing and converting the string to bits
            for (int i = 1; i < no_of_itr - 1; i++)
            {
                bitset<8> temp(msg_recieved[i]);
                msg_in_bits += temp.to_string();
                if (msg_recieved[i] != '/')
                {
                    temp_byte_stuffing += msg_recieved[i];
                }
                else if (msg_recieved[i] == '/' && is_esc)
                {
                    temp_byte_stuffing += msg_recieved[i];
                    is_esc = false;
                }
                else
                {
                    is_esc = true;
                }
            }
            bitset<8> end_byte(msg_recieved[0]);
            msg_in_bits += end_byte.to_string() + rmsg->getCrc().to_string().substr(1);

            // Recalculating the CRC of the received msg
            bitset<8> crc = calculateCRC(msg_in_bits, generator_bits);
            bitset<8> zeros("00000000");

            //check if any error happened in the transmission (by the rem of the CRC)
            int mtype = 0;
            if (crc == zeros)
            {
                mtype = 1;
            }
            else
            {
                mtype = 2;
            }
            Ack_Num = rmsg->getMsgID()+1;
            Rn = Ack_Num;
            handleACK(Ack_Num, mtype, newmsg);

            /*if (Ack_Num <= rmsg->getMsgID()) //less than for Lost case!
            {
                Ack_Num = rmsg->getMsgID() + 1;
                handleACK(Ack_Num, mtype, newmsg);
                //put that in file
                //cout << "At Receiver: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - next ACK " << rmsg->getPiggyBackingID() << endl;
            }
            else
            { //discard this frame (not proceeded to Network layer)
                cout << "Dup Case" <<endl;
                handleACK(Ack_Num, mtype, newmsg);
                //cout << "At Receiver(discarded) : at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - next ACK " << rmsg->getPiggyBackingID()  << endl;
            }*/

            ////////////////////////////////////////
            ///     Receive ACK -> Send Data     ///
            ////////////////////////////////////////

            if (dataMessages.size() <= Seq_Num)
            {
                cout << getName() << "    Finish sending!"
                     << "    <Recv Part> Rn=" <<to_string(Rn) <<" At "<< rmsg->getSendingTime() <<endl;
                /*cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;
                //TODO: signal the end of Node transmission

                outStream << "- ..................." << endl;
                outStream << "- end of input file" << endl;
                outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                outStream.close();*/
                string str = rmsg->getM_Payload();
                if(str == "Finish")//other node send "Finish" as a terminating flag (both now finished)
                {
                    cout << "We are DONE!!" <<endl;
                    endSimulation();
                }
                totalNumberOfTransmissions++;
                newmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
                newmsg->setM_Payload("Finish");
                sendDelayed(newmsg,propagationDelay, "out");
                return;
            }

            //if (Seq_Num == rmsg->getPiggyBackingID()){
                handleData(dataMessages[Seq_Num], Seq_Num, newmsg, rmsg->getSendingTime() - simTime().dbl());
                //then increment the seq. num. (msgID)
                Seq_Num++;
            //}
            /*else
            { //discard this ACK!

                cout << Seq_Num<< " - " << rmsg->getPiggyBackingID() << endl;
            }*/
            Sf = rmsg->getPiggyBackingID(); //start of frame
            Sn = Seq_Num;

            //printing:
            cout << getName() << "    <Send Part> -sent frame: "<< to_string(Sn-1)
                   << " Sf=" << Sf << " Sn=" << Sn << " sent ACK: " << newmsg->getPiggyBackingID()
                   << "    <Recv Part> Rn=" << Rn << " rec ACK: " << rmsg->getPiggyBackingID()<<" At "<< rmsg->getSendingTime() <<endl;


        }
    }
}
