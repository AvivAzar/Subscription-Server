#include <stdlib.h>
#include <iostream>
#include <thread>
#include "../include/ConnectionHandler.h"
#include "../include/StompProtocol.h"

int main(int argc, char *argv[])
{ 
	//Initialization
	ConnectionHandler *connectionHandler=new ConnectionHandler("",0);
	atomic<bool> *terminate = new atomic<bool>(false);
	atomic<bool> *inputReady = new atomic<bool>(false);
	string username;
	StompProtocol *stompProtocol= new StompProtocol;

	//Establishing connection
    const short bufsize = 1024;
    char buf[bufsize];
    cin.getline(buf, bufsize);
    string s(buf);
    int i = s.find(" ");
    int i2 = s.find(':');
    int i3 = s.find(' ', i2);
    int i4 = s.find(' ', i3 + 1);
    if (i != -1 && i2 != -1 && i3 != -1 && i4 != -1 && s.substr(0, i) == "login") { //#entire function is currently within this if     
            string host = s.substr(6, i2 - 6);
            short port = stoi(s.substr(i2 + 1, i3 - i2 - 1)); 
            connectionHandler->Set(host, port);
            string username = s.substr(i3 + 1, i4 - i3 - 1);
            stompProtocol->set(username);
            string password = s.substr(i4 + 1);
            string frame = "CONNECT\naccept-version:1.2\nhost:" + host + "\nlogin:" +
             username + "\npasscode: " + password + "\n\n" + '\0';
            if (!connectionHandler->connect()){ // creates physical connection
                std::cerr << "Could not connect to server";
                return 1;
            }

            if (!connectionHandler->sendLine(frame)){
                cout << "Disconnected. Exiting...\n"
                     << endl;
                *terminate = true;
                return 1;
            }
		

	thread serverThread([connectionHandler, stompProtocol, inputReady, terminate]()
						{			
		while (!*terminate)	{
			string answer;
			if (!connectionHandler->getLine(answer)){ // Supposed to read up to the null character
				cout << "Disconnected. Exiting...\n"
					 << endl;
				*terminate = true;
			}
			//int len = answer.length();
			// answer.resize(len - 1);
			answer=stompProtocol->ReceiveFrame(answer);
			*inputReady=true;
			if (answer == "ERROR")
			{
				std::cout << "Exiting...\n"
						  << std::endl;
				*terminate = true;
			}
			else if (answer == "CONNECTED")
			{
				cout<<"\nLogin successful\n";
			}
			else if (answer =="RECEIPT 0"){
				*inputReady=false;
				connectionHandler->close();
				*terminate=true;
			}
		}
        });

	thread userThread([stompProtocol, connectionHandler, terminate, inputReady]() { // User reading thread starts here by calling function
		while (!*terminate)
		{ // user reading thread here
		    if (*inputReady){
			const short bufsize = 1024;
			char buf[bufsize];		   // Aviv:I assume the buffer is created anew just to clear it from previous input
			cin.getline(buf, bufsize); // Aviv:reads from the client.
			string line(buf);
			string frame = stompProtocol->receiveUserCommand(line);
			if (frame.substr(0, 4) == "SEND")
			{ // a send type return might contain multiple frames
				while (!frame.empty())
				{
					int index = frame.find("SEND\n", 5);
					string temp;
					if (index != -1)
					{
						temp = frame.substr(0, index) + '\0'; // if there is another instance of SEND (meaning another event) in the string,
						// sends up to the index before it. if there isn't, sends the whole string.
					}
					else
					{
						temp = frame + '\0';
					}
					if (!connectionHandler->sendLine(temp))
					{
						cout << "Could not connect to server" << endl;
						*terminate = true;
					}
					else
					{
						if (index != -1)
							frame = frame.substr(index); // erases that frame from the string after it's processed.
						else
							frame = "";
					}
				}
			}
			//
			else if(frame.substr(0, 10) == "DISCONNECT"){
				if (!connectionHandler->sendLine(frame))
				{
					cout << "Could not connect to server" << endl;
					*terminate = true;
				}
				*inputReady=false;
			}
			//
			else if (!frame.empty() && frame != "?")
			{
				if (!connectionHandler->sendLine(frame))
				{
					cout << "Could not connect to server" << endl;
					*terminate = true;
				}
				*inputReady=false;
			}
		}
		}
	});
	userThread.join();
	serverThread.join();
	}
	delete inputReady;
	delete terminate;
	delete stompProtocol;
	delete connectionHandler;
	return 0;
}