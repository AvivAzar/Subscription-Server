
#include <string>
#include <iostream>
#include "../include/StompProtocol.h"
#include "../include/event.h"
#include "../include/ConnectionHandler.h"

using namespace std;

StompProtocol::StompProtocol():lastID(0),framesAwaitingReciept(),subToID(),IDToSub(),eventsByGameUser(),username(""){
}

void StompProtocol::set(string user){
    username=user;
}

string StompProtocol::ReceiveFrame(string frame){ //recieves a frame from the server- prints it and returns a string depending on the frame
    cout<<frame;
    int index=frame.find('\n');
    string command=frame.substr(0, index);
    if (command=="CONNECTED") return "CONNECTED";
    if (command=="MESSAGE"){
        updateGame(frame);
        return "MESSAGE";
        }
    if (command=="RECEIPT"){//Working clientside 10.1 for identifying Receipt frame and ID, but I'm not sure what we actually want it to do with that info.
        int index=frame.find(":");
        int last=frame.find('\n', index);
        int id=stoi(frame.substr(index+1,last-index-1));
        if (IDToSub.count(id)!=0){//check if its a subscription ID
            if (IDToSub[id].substr(0,1)!="\n") cout<<"\nJoined channel "+IDToSub[id]+"\n";
            else cout<<"\nExited channel "+IDToSub[id].substr(1)+'\n';
        }
        return "RECEIPT "+frame.substr(index+1,last-index-1); //id of the receipt
    }
    if (command=="ERROR"){//Working clientside 10.1
        if(frame.find("incorrect password")!=-1) cout<<"\nWrong password\n";
		else if(frame.find("User already logged in")!=-1) cout<<"\nUser already logged in\n";
        return "ERROR";
    }
    return("UNRECOGNIZED");    
}

string StompProtocol::receiveUserCommand(string command){
    if (command=="logout") return "DISCONNECT\nreceipt:0\n\n"+'\0';//Working clientside 10.1
    int index=command.find(' ');
    if (index!=-1){
    string header=command.substr(0, index);
    if (header=="join") return join (command.substr(index+1));//Working clientside 10.1
    if (header=="exit") return exit(command.substr(index+1));//Working clientside 10.1
    if (header=="report") return report(command.substr(index+1));//Working clientside 11.1
    if (header=="summary") return summary(command.substr(index+1));//Clientside printing test was fine, but need to see if it can also write to file properly
    if (header=="login") cout<< ("The client is already logged in, log out before trying again."); //Working clientside 10.1. stompProtocol is only accesisble after user has logged in for simplicity
    return "?";
    }
    return "?";
}

string StompProtocol:: join(string command){//subscribe frame
    subToID[command]=++lastID;
    IDToSub[lastID]=command;
    return "SUBSCRIBE\ndestination:"+command+"\nid:"+to_string(lastID)+"\nreceipt:"+to_string(lastID)+"\n\n"+'\0'; //one of the slashes being the other way is intentional, that's how the frame is described in the pdf
}

string StompProtocol:: exit(string command){//unsubscribe frame
    int id=subToID.at(command);
    IDToSub[++lastID]="\n"+command;//if exiting channel, adds \n before its name
    return "UNSUBSCRIBE\nid:"+to_string(id)+"\nreceipt:"+to_string(lastID)+"\n\n"+'\0';
}

string StompProtocol:: report(string command){ //send frame
    names_and_events parsedReport =parseEventsFile(command);
    string destination=parsedReport.team_a_name+'_'+parsedReport.team_b_name;
    string toReturn="";
    while(!parsedReport.events.empty()){
        toReturn=toReturn+"SEND\ndestination:"+destination+"\n\nuser: "+username+"\nteam a: "+parsedReport.events[0].get_team_a_name()+"\nteam b: "+parsedReport.events[0].get_team_b_name()+"\nevent name: "+parsedReport.events[0].get_name()+"\ntime: "+to_string(parsedReport.events[0].get_time())+"\ngeneral game updates:\n";
        for (map <string,string>:: const_iterator it = parsedReport.events[0].get_game_updates().begin(); it != parsedReport.events[0].get_game_updates().end(); ++it) {
            toReturn=toReturn+ it->first + ": " + it->second + '\n';
        }
        toReturn=toReturn+"\nteam a updates:\n";
        for (map <string,string>:: const_iterator it = parsedReport.events[0].get_team_a_updates().begin(); it != parsedReport.events[0].get_team_a_updates().end(); ++it) {
            toReturn=toReturn+ it->first + ": " + it->second + '\n';
        }
        toReturn=toReturn+"team b updates:\n";
        for (map <string,string>::const_iterator it = parsedReport.events[0].get_team_b_updates().begin(); it != parsedReport.events[0].get_team_b_updates().end(); ++it) {
            toReturn=toReturn+ it->first + ": " + it->second + '\n';
        }
        toReturn=toReturn+"description:\n"+parsedReport.events[0].get_discription()+"\n";
        parsedReport.events.erase(parsedReport.events.begin());
    }
    return toReturn;//The string contains all frames to be sent one after another, seperated by the string character.
}

string StompProtocol:: summary(string command){ //does not correspond to frame- client internal command
    int i1=command.find(' ');
    int i2=command.find (' ', i1+1);
    string game=command.substr(0,i1);
    string user=command.substr(i1+1, i2-i1-1);
    pair <string, string> gameUser (game, user);
    string file=command.substr (i2+1);
    ofstream fw(file, ofstream::out);
    if (fw.is_open()){
        for (auto Iter = eventsByGameUser[gameUser].begin(); Iter!=eventsByGameUser[gameUser].end(); Iter++){
            fw<<Iter->second;
        }
        fw.close();
        return "";
    }
    cout<< "ERROR WRITING TO FILE";
    return "ERROR WRITING TO FILE";
    
}

void StompProtocol:: updateGame(string events){
    string teamA=parseBetween(events,"destination:","_");
    string teamB=parseBetween(events,"_","\n");
    string game=teamA+"_"+teamB;
    
    string user=parseBetween(events,"user: ", "\n");
    pair<string,string> gameUser=pair <string, string> (game, user);
    if (eventsByGameUser[gameUser].empty()){
        vector <pair<int, string>> v;
        eventsByGameUser.insert(pair <pair <string, string>, vector<pair <int, string>>> (gameUser, v));
        string header=teamA+" vs "+teamB+"\nGame stats:\nGeneral stats:\n\n"+teamA+" stats:\n\n"+teamB+" stats:\n\n";
        eventsByGameUser[gameUser].push_back(pair <int, string> (-1, header));
        eventsByGameUser[gameUser].push_back(pair <int, string> (50000, ""));//creates an 'empty' event in the end of the list, so the iterator that sorts by time won't try to pass the last event
    }
    vector<pair<int, string>> *gameVector=&eventsByGameUser.at(gameUser);
    while (events!=""){
        //find indexes of every stat- general is always first, team A second, team B third, so it just searches for stats three times.
            int indexGeneralI=gameVector->at(0).second.find("General stats");//finds all relevant indexes both in the frame we're processing and in the string we're updating, to ensure all searches are done within the scope (for example, to avoid accidentially finding a description that says 'has taken possession of the ball' while looking for the possession stat)
            int indexAI=gameVector->at(0).second.find("stats", indexGeneralI+10);
            int indexBI=gameVector->at(0).second.find("stats", indexAI+5);
            int indexGeneralP=events.find("general game updates");
            int indexAP=events.find("team a updates");
            int indexBP=events.find("team b updates", indexAP+7);
            int indexDescP=events.find("description");
            int currIndex=events.find('\n',indexGeneralP);//used to go over the frame, pausing each time in the new line character

            string timep=parseBetween(events,"time: ", "\n");
            string event=timep+" - "+parseBetween(events,"event name: " ,"\n")+":\n\n";
            int removeLength=events.find("SEND\n", 5);//if there is another SEND in the string
            if (removeLength!=-1) event+=parseBetween(events, "description:" ,"SEND\n");
            else event+=events.substr(indexDescP+12);
            gameVector->push_back(pair <int, string> (stoi(timep), event));

            //get and place general updates
            while (currIndex<indexAP-1) {
                int headerEnd=events.find(':',currIndex);
                if (headerEnd!=-1 && headerEnd<indexAP) {
                    string currHeader=events.substr(currIndex+1, headerEnd-currIndex-1);
                    int toReplace=gameVector->at(0).second.find(currHeader, indexGeneralI);
                    string toAdd=events.substr(headerEnd+2, events.find('\n', headerEnd)-headerEnd-2);//returns the value that is to be updated to the key
                    if(toReplace!=-1 && toReplace<indexAI){ //checks if value exists already
                        int statStartIndex=toReplace+currHeader.length()+2; //#need to check these two are actually the correct values both here and in substr use
                        int statEndIndex=gameVector->at(0).second.find('\n', statStartIndex);
                        gameVector->at(0).second=gameVector->at(0).second.substr(0, statStartIndex)+toAdd+gameVector->at(0).second.substr(statEndIndex);
                    }
                    else{
                        gameVector->at(0).second=gameVector->at(0).second.substr(0,indexGeneralI+15)+currHeader +": "+toAdd +'\n'+gameVector->at(0).second.substr(indexGeneralI+15);
                        indexAI+=currHeader.length()+toAdd.length()+3;
                        indexBI+=currHeader.length()+toAdd.length()+3;
                    }
                }
                currIndex=events.find('\n',currIndex+1);
            }
            
            currIndex=events.find('\n',currIndex+1);//skip reading the team A updates line
            //get and place team a updates
            while (currIndex<indexBP-1) {
                int headerEnd=events.find(':',currIndex);
                if (headerEnd!=-1 && headerEnd<indexBP) {
                    string currHeader=events.substr(currIndex+1, headerEnd-currIndex-1);
                    int toReplace=gameVector->at(0).second.find(currHeader, indexAI);
                    string toAdd=events.substr(headerEnd+2, events.find('\n', headerEnd)-headerEnd-2);//returns the value that is to be updated to the key
                    if(toReplace!=-1 && toReplace<indexBI){ //checks if value exists already
                        int statStartIndex=toReplace+currHeader.length()+2;
                        int statEndIndex=gameVector->at(0).second.find('\n', statStartIndex);
                        gameVector->at(0).second=gameVector->at(0).second.substr(0, statStartIndex)+toAdd+gameVector->at(0).second.substr(statEndIndex);
                    }
                    else{
                        gameVector->at(0).second=gameVector->at(0).second.substr(0,indexAI+7)+currHeader +": "+toAdd +'\n'+gameVector->at(0).second.substr(indexAI+7);
                        indexBI+=currHeader.length()+toAdd.length()+3;
                    }
                }
                currIndex=events.find('\n',currIndex+1);
            }

            currIndex=events.find('\n',currIndex+1);//skip reading the team B updates line
            //get and place team b updates
            while (currIndex<indexDescP-1) {//-1 because unlike searching for (general, team a, teamb) updates, description points to the first index of its line
                int headerEnd=events.find(':',currIndex);//#Cuurently also puts description in team B updates, even though its not supposed to
                if (headerEnd!=-1 && headerEnd<indexDescP) {
                    string currHeader=events.substr(currIndex+1, headerEnd-currIndex-1);
                    int toReplace=gameVector->at(0).second.find(currHeader, indexBI);
                    string toAdd=events.substr(headerEnd+2, events.find('\n', headerEnd)-headerEnd-2);//returns the value that is to be updated to the key
                    if(toReplace!=-1){ //checks if value exists already
                        int statStartIndex=toReplace+currHeader.length()+2;
                        int statEndIndex=gameVector->at(0).second.find('\n', statStartIndex);
                        gameVector->at(0).second=gameVector->at(0).second.substr(0, statStartIndex)+toAdd+gameVector->at(0).second.substr(statEndIndex);
                    }
                    else{
                        gameVector->at(0).second=gameVector->at(0).second.substr(0,indexBI+7)+currHeader +": "+toAdd +'\n'+gameVector->at(0).second.substr(indexBI+7);
                    }
                }
                currIndex=events.find('\n',currIndex+1);
            }
            if (removeLength!=-1){
                events=events.substr(removeLength); //removes processed event, and checks it will not go out of bounds
                timep=stoi(parseBetween(events, "time: ", "\n"));
            } 
            else {
                events="";
            } 
        }
    }

string StompProtocol::parseBetween(string toParse, string preFirst, string afterLast){
    int indexS=toParse.find(preFirst)+preFirst.length();
    int indexE=toParse.find(afterLast, indexS);
    if (indexS<(int)preFirst.length() || indexE<0) return "";
    return (toParse.substr(indexS, indexE-indexS));
}
