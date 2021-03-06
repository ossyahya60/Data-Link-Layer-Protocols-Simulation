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

vector<int> Node::generateHammingCode(vector<int> msgBits, int m, int r)
{
    // Stores the Hamming Code
    vector<int> hammingCode(r + m);
    // Find positions of redundant bits
    for (int i = 0; i < r; ++i)
    {
        // Placing -1 at redundant bits place to identify it later
        hammingCode[pow(2, i) - 1] = -1;
    }
    int j = 0;
    // Iterate to update the code
    for (int i = 0; i < (r + m); i++)
    {
        // Placing msgBits where -1 is absent i.e., except redundant bits all positions are msgBits
        if (hammingCode[i] != -1)
        {
            hammingCode[i] = msgBits[j];
            j++;
        }
    }
    for (int i = 0; i < (r + m); i++)
    {
        // If current bit is not redundant bit then continue
        if (hammingCode[i] != -1)
            continue;

        int x = log2(i + 1);
        int one_count = 0;
        // Find msg bits containing set bit at x'th position
        for (int j = i + 2; j <= (r + m); ++j)
        {
            if (j & (1 << x))
            {
                if (hammingCode[j - 1] == 1)
                {
                    one_count++;
                }
            }
        }
        // Generating hamming code for even parity
        if (one_count % 2 == 0)
        {
            hammingCode[i] = 0;
        }
        else
        {
            hammingCode[i] = 1;
        }
    }
    // Return the generated code
    return hammingCode;
}

// Function to find the hamming code of the given message bit msgBit[]
vector<int> Node::findHammingCode(vector<int>& msgBits)
{
    // Message bit size
    int m = msgBits.size();
    // r is the number of redundant bits
    int r = 1;
    // Find no. of redundant bits
    while (pow(2, r) < (m + r + 1))
    {
        r++;
    }
    // Generating Code
    vector<int> hamming = generateHammingCode(msgBits, m, r);
    return hamming;
}

vector<int> Node::generateMsg(vector<int> hamming, int m, int r)
{
    // Stores the message
    vector<int> msg;
    // Stores indices of redundant bits
    vector<int> positionRedundant;
    // Find positions of redundant bits
    for (int i = 0; i < r; ++i)
    {
        // Placing index of redundant bit in a vector for later use
        positionRedundant.push_back(pow(2, i) - 1);
    }
    for (int i = 0; i < hamming.size(); i++)
    {
        // If current index in hamming code is not index of redundant bit put it in msg
        if (find(positionRedundant.begin(), positionRedundant.end(), i) == positionRedundant.end())
        {
            msg.push_back(hamming[i]);
        }
    }
    return msg;
}

vector<int> Node::findMsg(vector<int>& hamming)
{
    // Hamming code bit size
    int size = hamming.size();
    // r is the number of redundant bits
    int r = 1;
    // Find no. of redundant bits
    while (pow(2, r) < (size + 1))
    {
        r++;
    }
    //cout << r << endl;
    vector<int> msgBits = generateMsg(hamming, size - r, r);
    return msgBits;
}

int Node::receiveHamming(vector<int> hamming, bool& error)
{
    // Hamming code bit size
    int size = hamming.size();
    // r is the number of redundant bits
    int r = 1;
    // Find no. of redundant bits
    while (pow(2, r) < (size + 1))
    {
        r++;
    }
    vector<int> positionRedundant;
    // Find positions of redundant bits
    for (int i = 0; i < r; ++i)
    {
        positionRedundant.push_back(pow(2, i) - 1);
    }
    // change 1 bit in code
    //hamming[0] = 1;
    for (int i = 0; i < size; i++)
    {
        // If current bit is not redundant bit then continue
        if (find(positionRedundant.begin(), positionRedundant.end(), i) == positionRedundant.end())
            continue;

        int x = log2(i + 1);
        int one_count = 0;
        // Find msg bits containing set bit at x'th position
        for (int j = i + 2; j <= size; ++j)
        {
            if (j & (1 << x))
            {
                if (hamming[j - 1] == 1)
                {
                    one_count++;
                }
            }
        }
        // Generating hamming code for even parity
        if (one_count % 2 == 0)
        {
            hamming[i] = hamming[i] ^ 0;
        }
        else
        {
            hamming[i] = hamming[i] ^ 1;
        }
    }
    string errorPos;
    for (int i = 0; i < r; ++i)
    {
        errorPos += to_string(hamming[positionRedundant[i]]);
        //cout << hamming[positionRedundant[i]] << endl;
    }
    //cout << errorPos << endl;
    reverse(errorPos.begin(),errorPos.end());
    char * pEnd;
    int errorPosition = strtoull(errorPos.c_str(), &pEnd, 2);
    //cout << errorPosition << endl;
    if (errorPosition == 0)
    {
        return -1;
    }
    else
    {
        error = true;
        return errorPosition - 1;
    }
}

void Node::handleData(pair<string, string> msgPair, int seqNo, MyMessage_Base *rmsg, double betweenFramesDelay, bool receivedNACK)
{
    string errorBits = msgPair.first;
    string msgText = msgPair.second;
    double delay = 0;
    bool duplicated = false;
    //new msg for sending:
    MyMessage_Base *nmsg = new MyMessage_Base();
    nmsg->setMsgID(seqNo);
    nmsg->setPiggyBackingID(rmsg->getPiggyBackingID());
    nmsg->setM_Type(rmsg->getM_Type());
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);


    string newMsgText = "";
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

    int errorHandle = getParentModule()->par("errorHandle").intValue(); //get it from .ini file
    if (errorHandle == 0)
    {
        //Set the CRC of the msg
            bitset<8> generator_bits(generator);
            bitset<8> crc = calculateCRC(msg_in_bits, generator_bits);
            nmsg->setCrc(crc);
    }
    else
    {
        vector<int> msgBits;
        bitset<8> byte;
        for (int i = 0; i < msgText.size(); ++i)
        {
            byte= msgText.c_str()[i];
            //cout << byte << endl;
            for (int j = 7; j > -1; j--)
            {
                msgBits.push_back(byte[j]);
            }
        }

        // Function Call
        vector<int> hamming = findHammingCode(msgBits);

        cout << "Message bits are:           ";
        for (int i = 0; i < msgBits.size(); i++)
            cout << msgBits[i] << " ";
        cout << endl;

        cout << "Hamming code is:            ";
        for (int i = 0; i < hamming.size(); i++)
            cout << hamming[i] << " ";
        cout << endl;

        if(!receivedNACK && errorBits[0] == '1'){
            int randIndex = std::rand() % hamming.size(); // get random index of text message to error it
            hamming[randIndex] = (hamming[randIndex] == 0) ? 1 : 0;
        }

        //int numChar = hamming.size();
        int j = 7;
        msgText = "";
        for (int i = 0; i < hamming.size(); i++)
        {
            //if (numChar == 0)
                //break;
            byte[j] = hamming[i];
            j--;
            if(j == -1)
            {
                msgText += byte.to_ulong();
                //numChar--;
                j = 7;
                //cout << byte.to_ulong() << endl;
                byte = 0;
            }
        }
        cout << "After encoding: " << msgText << endl;
    }

    string errors = "";

    if (!receivedNACK && errorBits[0] == '1') //modification
    {
        errors += errors.length() > 0 ? ", modification" : "modification";
        if(errorHandle == 0){
            int randChar = std::rand() % msgText.length(); // get random index of text message to error it
            //bitset<8> rem_in_bits(msgText[randChar]);
            msgText[randChar] ^= (std::rand() % 8) +1;
        }

    }
    if (!receivedNACK && errorBits[1] == '1') //loss
    {
        errors += errors.length() > 0 ? ", loss" : "loss";
        outStream << "- " << getName() << " loss the frame with message id=" << nmsg->getMsgID() << " at " << simTime() + delay + betweenFramesDelay + propagationDelay << " piggyId = " << nmsg->getPiggyBackingID()<<endl;
        cout << getName() << " lost for message id=" << nmsg->getMsgID() << " at " << simTime() + delay + betweenFramesDelay + propagationDelay << endl;
        return;
    }
    if (!receivedNACK && errorBits[2] == '1') //duplication
    {
        errors += errors.length() > 0 ? ", duplication" : "duplication";
        duplicated = true;
    }
    if (!receivedNACK && errorBits[3] == '1') //delay
    {
        errors += errors.length() > 0 ? ", delay" : "delay";
        delay = getParentModule()->par("delay").doubleValue(); //get it from .ini file
    }

    nmsg->setM_Payload(msgText.c_str());

    totalNumberOfTransmissions++;

    nmsg->setSendingTime(simTime().dbl() + delay + betweenFramesDelay + propagationDelay);
    nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
    outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
              << " to " << simTime() + delay + betweenFramesDelay + propagationDelay << " with " << ((errors.length() > 0) ? errors : "no errors") << " piggyId = " << nmsg->getPiggyBackingID()<<endl;
    sendDelayed(nmsg, propagationDelay+delay +betweenFramesDelay, "out");


    if (duplicated) /*&& !lost)*/ //8aleban hanfkes ll duplication if msg was lost
    {
        MyMessage_Base *dupmsg = new MyMessage_Base();
        dupmsg->setMsgID(seqNo);
        dupmsg->setM_Type(nmsg->getM_Type()); // 0 -> Data
        dupmsg->setSendingTime(simTime().dbl() + delay + 0.01 + betweenFramesDelay + propagationDelay);
        dupmsg->setM_Payload(msgText.c_str());
        totalNumberOfTransmissions++;
        nmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
        outStream << "- " << getName() << " sends message with id=" << nmsg->getMsgID() << " and content=\"" << nmsg->getM_Payload() << "\""
                  << " to " << simTime() + delay + betweenFramesDelay + propagationDelay + 0.01 << " with " << ((errors.length() > 0) ? errors : "no errors") << " piggyId = " << nmsg->getPiggyBackingID()<<endl;
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
    string pload = rmsg->getM_Payload();
    if(pload != "Finish" && pload != "start"){
        outStream << "- " << getName() << " received message with id=" << rmsg->getMsgID() << " and content=\"" << pload << "\""
                                          << " at " << simTime() << " piggyId = " << rmsg->getPiggyBackingID()<<endl;
    }

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
    //TODO: if I've finished make we can choose if using default windowSize or calculated windowSize from dataMessages:
    int windowSize = getParentModule()->par("windowSize").intValue();
    // TODO - Generated method body
    if (msg->isSelfMessage())
    {
        cout << getName() << " will start sending:" <<endl;
        MyMessage_Base* rmsg = check_and_cast<MyMessage_Base *>(msg);
        rmsg->setM_Payload("start");
        Ack_Num = rmsg->getPiggyBackingID();
        Rn = Ack_Num;
        handleACK(Ack_Num, 1, rmsg);

        if (dataMessages.size() <= Seq_Num)
        {
            //TODO: signal the end of Node transmission

            string str = rmsg->getM_Payload();
            if(str == "Finish")//other node send "Finish" as a terminating flag (both now finished)
            {
                cout << "We are DONE!!" <<endl;
                outStream << "- ..................." << endl;
                outStream << "- " <<getName()<< " end of input file" << endl;
                outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;
                outStream.close();
                //endSimulation();
                return;
            }
            //outStream <<"- " << getName() << " received message with id " <<rmsg->getMsgID() << " " << str <<" At "<< rmsg->getSendingTime() <<endl;
            totalNumberOfTransmissions++;
            rmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
            rmsg->setM_Payload("Finish");
            sendDelayed(rmsg,propagationDelay, "out");
            return;
        }
        for(int i=0; i<windowSize; ++i){ //only for first time:
            handleData(dataMessages[Seq_Num], Seq_Num, rmsg,0.05*i,false);
            sentArr[Seq_Num] = true;
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
            int dataSize = dataMessages.size();
            for(int i=0; i<dataSize; ++i){
                sentArr.push_back(false);
            }
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

            int mtype = 1;
            int errorHandle = getParentModule()->par("errorHandle").intValue(); //get it from .ini file
            if (errorHandle == 0)
            {
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
                mtype = (crc == zeros) ? 1 : 2;
            }
            else
            {
                cout << "Received hamming code: " << msg_recieved << endl;
                vector<int> hamming;
                bitset<8> byte;
                for (int i = 0; i < msg_recieved.size(); ++i)
                {
                    byte= msg_recieved.c_str()[i];
                    //cout << byte << endl;
                    for (int j = 7; j > -1; j--)
                    {
                        hamming.push_back(byte[j]);
                    }
                }

                bool error = false;
                int errorPos = -1;
                vector<int> correctedMsg;
                errorPos = receiveHamming(hamming, error);
                if (error)
                {
                    cout << "Error at bit position: " << errorPos + 1 << endl;
//                    cout << "Hamming code with error is: ";
//                    for (int i = 0; i < hamming.size(); i++)
//                        cout << hamming[i] << " ";
//                    cout << endl;
                    if (hamming[errorPos] == 0)
                    {
                        hamming[errorPos] = 1;
                    }
                    else
                    {
                        hamming[errorPos] = 0;
                    }

//                    cout << "Corrected Hamming code is:  ";
//                    for (int i = 0; i < hamming.size(); i++)
//                        cout << hamming[i] << " ";
//                    cout << endl;
//
                      correctedMsg = findMsg(hamming);
                      //cout << "Corrected message: " << correctedMsg << endl;
//                    cout << "Corrected Message bits are: ";
//                    for (int i = 0; i < correctedMsg.size(); i++)
//                        cout << correctedMsg[i] << " ";
//                    cout << endl;
                }
                correctedMsg = findMsg(hamming);

                string receivedMsg;
                int numChar = correctedMsg.size() / 8;
                int j = 7;
                for (int i = 0; i < correctedMsg.size(); i++)
                {
                    if (numChar == 0)
                        break;
                    byte[j] = correctedMsg[i];
                    j--;
                    if(j == -1)
                    {
                        receivedMsg += byte.to_ulong();
                        numChar--;
                        j = 7;
                        //cout << byte.to_ulong() << endl;
                        byte = 0;
                    }
                }
                cout << "After decoding: " << receivedMsg << endl;
            }


            //check for receiving:
            if(Rn == rmsg->getMsgID())//-> correct; what I am expecting
            {
                //send data normal with its ack
                arrived[rmsg->getMsgID()-Rn]=true;
                //if(mtype!=2){
                bool allRec = true;
                for(int m=0;m<max_arrived_id -Rn;m++){
                    if(!arrived[m])
                        allRec = false;

                }
                    for(int k=0; k<windowSize;++k){
                        if(arrived[k]){ //arrived = [t,t,t,f] [f,t,t,f]
                            Ack_Num++;
                            arrived[k]=false;   //arrived[0]=false
                        }
                        else{   //arrived = [f,t,t,f]
                            cout << "hereeeeeee" <<endl;
                            for(int j=k+1;j<windowSize-1;++j){
                                arrived[j-1]=arrived[j];
                            }
                            arrived[windowSize-1]=false;
                            break;
                        }
                    }
                //}
                //Ack_Num = rmsg->getMsgID()+1;
                sentNACK =false;
                Rn = Ack_Num;
                handleACK(Ack_Num, mtype, newmsg);
            }
            else if(Rn < rmsg->getMsgID() && Rn + windowSize > rmsg->getMsgID())// && !sentNACK)//->loss or delay
            {
                max_arrived_id = rmsg->getMsgID();
                //send NACK and buffer this frame
                cout << "Lost/Delay case: Rn= " <<Rn << " - rec msgId= " <<rmsg->getMsgID() <<endl;
                arrived[rmsg->getMsgID()-Rn]=true; //arr = [f,f,t,t]
                if(!sentNACK){
                    mtype=2; //NACK
                }
                else{
                    mtype=1;
                }
                sentNACK =true; //send NACK only one time then wait for time=2*betweenFrameDelay before sending another NACK again if needed
                //arrived[rmsg->getMsgID()] = true;
                Rn = Ack_Num;
                handleACK(Ack_Num, mtype, newmsg);
            }
            else //->either Rn > msgId received or msgId > Rn and out of Window size of buffer
            {
                //discarded
                string nn = getName();
                if(rmsg->getMsgID() == 1 && nn=="node2"){
                    cout << "DUPLICATEEEEEEEEEEEEEE rec node2" << " with msg id = " <<newmsg->getMsgID() <<endl;
                    cout << "- " << getName() << " received message with id=" << newmsg->getMsgID() << " and content=\"" << newmsg->getM_Payload() << "\""
                                                      << " at " << simTime() << endl;

                }


                //cout << "Discarded case: Rn= " <<Rn << " - rec msgId= " <<rmsg->getMsgID() <<endl;
                cout <<getName() << " "<<rmsg->getM_Payload() <<" - Discarded!!" <<endl;
                sentNACK =false;
            }


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

            /*if (dataMessages.size() <= Seq_Num)
            {
                cout << getName() << "    Finish sending!"
                     << "    <Recv Part> Rn=" <<to_string(Rn) <<" At "<< rmsg->getSendingTime() <<endl;
                cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;
                //TODO: signal the end of Node transmission

                outStream << "- ..................." << endl;
                outStream << "- end of input file" << endl;
                outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                outStream.close();
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
            }*/

            //cout << "mtype of rcv msg = " <<rmsg->getM_Type()<<endl;
            if (rmsg->getM_Type() == 2){//received NACK
                cout << "will resend lost frame = " << rmsg->getPiggyBackingID()<<endl;
                if (dataMessages.size() <= rmsg->getPiggyBackingID())
                {
                    cout << getName() << "    Finish sending!"
                         << "    <Recv Part> Rn=" <<to_string(Rn) <<" At "<< rmsg->getSendingTime() <<endl;
                    //cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;

                    string str = rmsg->getM_Payload();
                    if(str == "Finish")//other node send "Finish" as a terminating flag (both now finished)
                    {
                        cout << "We are DONE!!" <<endl;
                        //TODO: signal the end of Node transmission
                        outStream << "- ..................." << endl;
                        outStream << "- " <<getName()<< " end of input file" << endl;
                        outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                        outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                        outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                        outStream.close();
                        //endSimulation();
                        return;
                    }
                    totalNumberOfTransmissions++;
                    //outStream <<"- " << getName() << " received message with id " <<rmsg->getMsgID() << " " << str <<" At "<< rmsg->getSendingTime() <<endl;
                    newmsg->setM_Payload("Finish");
                    sendDelayed(newmsg,propagationDelay, "out");
                    return;
                }
                //update ACK_Num
                cout << "Before ACK_Num : " <<  Ack_Num <<endl;
//                int l=Ack_Num;
//                while(sentArr[l]){
//                        Ack_Num++;
//                        l++;
//                }
                //Ack_Num = rmsg->getPiggyBackingID();
                //Rn = Ack_Num;
                cout << "After ACK_Num : " <<  Ack_Num <<endl;
                //newmsg->setPiggyBackingID(Ack_Num);


                handleData(dataMessages[rmsg->getPiggyBackingID()], rmsg->getPiggyBackingID(), newmsg, rmsg->getSendingTime() - simTime().dbl(),true);
                sentArr[rmsg->getPiggyBackingID()] = true;

                //Sn = Seq_Num;
                cout << "resend lost frame"<<endl;
                //printing:
                cout << getName() << "    <Send Part> -sent frame: "<< to_string(Sn-1)
                       << " Sf=" << Sf << " Sn=" << Sn << " sent ACK: " << newmsg->getPiggyBackingID()
                       << "    <Recv Part> Rn=" << Rn << " rec ACK: " << rmsg->getPiggyBackingID()<<" At "<< rmsg->getSendingTime() <<endl;

            }
            else{ //postive ACK
                Sf = rmsg->getPiggyBackingID(); //start of frame.
//                int j =0;
//                while(sentArr[j]){
//                    j++;
//                }
                string nodenamee = getName();
                if(nodenamee == "node5"){
                    cout << "startframe: " <<Sf <<endl;
                }
                /*int j=Sf;
                while(sentArr[j]){
                    j++;
                }
                Sf = j;*/
                //send all available in the frame:
                int dly=0;
                for(int i=Sf; i<windowSize+Sf; ++i){
                    if (dataMessages.size() <= Seq_Num)
                    {
                        cout << getName() << "    Finish sending!"
                             << "    <Recv Part> Rn=" <<to_string(Rn) <<" At "<< rmsg->getSendingTime() <<endl;
                        //cout << "At Sender: at time = " << rmsg->getSendingTime() << " : " << rmsg->getM_Payload() << " - " << rmsg->getPiggyBackingID() << " but we are DONE!!" << endl;

                        string str = rmsg->getM_Payload();
                        if(str == "Finish")//other node send "Finish" as a terminating flag (both now finished)
                        {
                            cout << "We are DONE!!" <<endl;
                            //check if i closed file dont reclose it:

                            //endSimulation();
                            outStream <<"- ..................." << endl;
                            outStream << "- " <<getName() <<" end of input file" << endl;
                            outStream << "- total transmission time= " << simTime().dbl() - startTime << endl;
                            outStream << "- total number of transmissions= " << totalNumberOfTransmissions + rmsg->getNumberOfTransmissions() << endl;
                            outStream << "- the network throughput= " << dataMessages.size() / (simTime().dbl() - startTime) << endl;

                            outStream.close();

                            return;
                        }
                        totalNumberOfTransmissions++;
                        //outStream <<"- " << getName() << " received message with id " <<rmsg->getMsgID() << " " << str <<" At "<< rmsg->getSendingTime() <<endl;
                        newmsg->setNumberOfTransmissions(totalNumberOfTransmissions);
                        newmsg->setM_Payload("Finish");
                        sendDelayed(newmsg,propagationDelay, "out");
                        return;
                    }
                    if(sentArr[i]) //was sent before
                    {
                        continue;
                    }

                    handleData(dataMessages[Seq_Num], Seq_Num, newmsg, dly*0.05,false);
                    if(newmsg->getM_Type() == 2){
                        newmsg->setM_Type(1);
                    }

                    sentArr[Seq_Num] = true;
                    //then increment the seq. num. (msgID)
                    Seq_Num++;

                    Sn = Seq_Num;

                    //printing:
                    cout << getName() << "    <Send Part> -sent frame: "<< to_string(Sn-1)
                           << " Sf=" << Sf << " Sn=" << Sn << " sent ACK: " << newmsg->getPiggyBackingID()
                           << "    <Recv Part> Rn=" << Rn << " rec ACK: " << rmsg->getPiggyBackingID()<<" At "<< to_string(rmsg->getSendingTime() + dly*0.05) <<endl;
                    dly++;
                }
            }
            /*else
            { //discard this ACK!

                cout << Seq_Num<< " - " << rmsg->getPiggyBackingID() << endl;
            }*/


//            for(int j =0;j<windowSize;++j){
//                arrived[j]=false;
//            }
        }
    }
}
