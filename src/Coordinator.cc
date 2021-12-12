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

#include "Coordinator.h"
#include "MyMessage_m.h"
Define_Module(Coordinator);

void Coordinator::initialize()
{
    // read from coordinator file:
    ifstream file("../files/coordinator.txt");
    string line;
    //file >> line;
    while(getline(file,line)){
        if(line[0] == ' ' || line[0] == '\r' || line[0] == '\n')
            continue;

        //cout << line <<endl;
        char* ln = &line[0];
        char *token = strtok(ln, " ");

        // Keep printing tokens while one of the
        // delimiters present in str[].
        vector<string> arr;
        while (token != NULL)
        {
            string tk = token;
            arr.push_back(tk);
            token = strtok(NULL, " ");
        }

        MyMessage_Base* msg= new MyMessage_Base();
        msg->setM_Payload(arr[1].c_str());
        msg->setM_Type(-1);
        //search for "start" in line:
        if(count(arr.begin(), arr.end(), "start")>0){ //sender
            //cout << "sender!!" << endl;
            msg->setSeq_Num(stoi(arr[3])); //TODO: handle new line problem (at the end of file)
            send(msg, "out", stoi(arr[0]));
        }
        else if(arr.size() > 0 && arr[0] != " ") { //receiver
            msg->setSeq_Num(-1);
            send(msg, "out", stoi(arr[0]));
        }
        /*if(arr.size() > 2){ //sender
            cout << "sender!!" << endl;
        }
        else if(arr.size() > 0) { //receiver
            cout << "receiver!!" << endl;
        }*/
        arr.clear();
    }

    file.close();
}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
