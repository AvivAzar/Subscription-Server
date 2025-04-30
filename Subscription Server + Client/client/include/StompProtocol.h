#pragma once
#include "../include/ConnectionHandler.h"
#include "../include/event.h"
#include <fstream>
using namespace std;

// TODO: implement the STOMP protocol
class StompProtocol
{
private:
    int lastID=-1; //for receipts
    vector<string> framesAwaitingReciept;
    map<string, int> subToID; //to keep track of the random IDs assigned to subscriptions to a specific game
    map<int, string> IDToSub; //opposite of the above
    map<pair <string, string>, vector<pair <int, string>>> eventsByGameUser; //pair of two strings is a game/user combination. second pair is a pair of string gamevector is a list of time & event pairs for that game, by that user.
    string username;
public:

StompProtocol();

void set(string user);

string ReceiveFrame(string frame);

string receiveUserCommand(string command);

string join(string command); //subscribe frame

string exit(string command); //unsubscribe frame

string report(string command); //send frame

string summary(string command); //does not correspond to frame- client internal command

void updateGame(string events);

string parseBetween(string toParse, string preFirst, string afterLast);

};
