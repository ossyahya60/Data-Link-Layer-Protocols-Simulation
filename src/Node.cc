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

void Node::handleSendMsg(pair<string, string> msgPair, int seqNo, MyMessage_Base *rmsg)
{
    string errorBits = msgPair.first;
    string msgText = msgPair.second;
    double delay = 0;
    bool duplicated = false;
    //new msg for sending:
    MyMessage_Base *nmsg = new MyMessage_Base();
    nmsg->setMsgID(seqNo);
    nmsg->setM_Type(0); // 0 -> Data
    nmsg->setSendingTime(simTime().dbl());
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
        //handle loss -> put on log file only:
        cout << "Lost message: at time = " << simTime().dbl() << " : " << msgText << " - " << seqNo << endl;
        int timeout = getParentModule()->par("timeout").intValue();
        MyMessage_Base *selfmsg = new MyMessage_Base();

        totalNumberOfTransmissions++;
        selfmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " timeout for message id=" << nmsg->getMsgID() << " at " << simTime() + timeout << endl;

        scheduleAt(simTime() + timeout, selfmsg);
        return;
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
    if (delay > 0)
    {
        totalNumberOfTransmissions++;
        nmsg->setSendingTime(simTime().dbl() + delay);
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
                  << " at " << simTime() + delay << " with " << ((errors.length() > 0) ? errors : "no errors") << endl;
        sendDelayed(nmsg, delay, "out");
    }
    else
    {
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
                  << " at " << simTime() << " with " << ((errors.length() > 0) ? errors : "no errors") << endl;
        send(nmsg, "out");
    }

    if (duplicated)
    {
        MyMessage_Base *dupmsg = new MyMessage_Base();
        dupmsg->setMsgID(seqNo);
        dupmsg->setM_Type(0); // 0 -> Data
        dupmsg->setSendingTime(simTime().dbl() + delay + 0.01);
        dupmsg->setM_Payload(msgText.c_str());
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
                  << " at " << simTime() + delay + 0.01 << " with " << ((errors.length() > 0) ? errors : "no errors") << endl;
        sendDelayed(dupmsg, delay + 0.01, "out");
    }
}

void Node::handleRecieveMsg(int ackNo, int mtype, MyMessage_Base *rmsg)
{
    string ack = "";
    if (mtype == 2)
    {
        ack = "NACK";
    }
    else
    {
        ack = "ACK";
    }
    MyMessage_Base *nmsg = new MyMessage_Base();
    nmsg->setM_Payload(ack.c_str());
    nmsg->setPiggyBackingID(ackNo);
    nmsg->setM_Type(mtype); // 1 -> ACK , 2 -> NACK
    nmsg->setSendingTime(simTime().dbl() + 0.2);
    totalNumberOfTransmissions++;
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    outStream << "- " << getName() << " received message with id=" << rmsg->getMsgID() << " and content=\"" << rmsg->getM_Payload() << "\""
              << " at " << simTime() << endl;
    sendDelayed(nmsg, 0.2, "out"); //TODO: handle delay time correctly
}

void Node::initialize()
{

}

void Node::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
    if (msg->isSelfMessage())
    {
        if (dataMessages.size() <= Seq_Num)
        {
            //TODO: signal the end of Node transmission
            MyMessage_Base *rmsg = check_and_cast<MyMessage_Base *>(msg);

            outStream << "- ..................." << endl;
            outStream << "- end of input file" << endl;
            outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
            outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
            outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

            outStream.close();

            return;
        }

        handleSendMsg(dataMessages[Seq_Num], Seq_Num, check_and_cast<MyMessage_Base *>(msg));
        //then increment the seq. num. (msgID)
        Seq_Num++;
    }
    else
    {
        MyMessage_Base *rmsg = check_and_cast<MyMessage_Base *>(msg);
        int initTime = rmsg->getMsgID();
        if (rmsg->getM_Type() == -1 && initTime != -1)
        { //node sent from coordinator
            string nodeName = getName();
            nodeName = nodeName.substr(4, nodeName.length() - 4);
            // TODO - Generated method body
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

            MyMessage_Base *selfmsg = new MyMessage_Base();
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
        else if (rmsg->getM_Type() != -1)
        { //msg from node (a peer)

            if (isSender)
            {

                if (dataMessages.size() <= Seq_Num)
                {
                    cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;
                    //TODO: signal the end of Node transmission

                    outStream << "- ..................." << endl;
                    outStream << "- end of input file" << endl;
                    outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                    outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                    outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                    outStream.close();

                    return;
                }

                if (Seq_Num == rmsg->getPiggyBackingID())
                {
                    //put that in the file
                    cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << endl;
                    handleSendMsg(dataMessages[Seq_Num], Seq_Num, rmsg);
                    //then increment the seq. num. (msgID)
                    Seq_Num++;
                }
                else
                { //discard this ACK!
                    cout << "At Sender(discarded): at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << endl;
                }
            }
            else
            {
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

                if (Ack_Num <= rmsg->getMsgID()) //less than for Lost case!
                {
                    Ack_Num = rmsg->getMsgID() + 1;
                    handleRecieveMsg(Ack_Num, mtype, rmsg);
                    //put that in file
                    cout << "At Receiver: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getMsgID() << endl;
                }
                else
                { //discard this frame (not proceeded to Network layer)
                    handleRecieveMsg(Ack_Num, mtype, rmsg);
                    cout << "At Receiver(discarded) : at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getMsgID() << endl;
                }
            }
        }
        else //receiver from coordinator
        {
            string nodeName = getName();
            nodeName = nodeName.substr(4, nodeName.length() - 4);
            // TODO - Generated method body
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
        }
    }
}
