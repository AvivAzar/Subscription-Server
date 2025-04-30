package bgu.spl.net.impl.stomp;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.srv.Connections;
import java.util.*;
import java.util.HashMap;
import bgu.spl.net.srv.ConnectionsImpl;

public class StompMessagingProtocolImpl implements StompMessagingProtocol<String> {

    private String frame;

    private ConnectionsImpl<String> connections;

    private int connectionId;

    private volatile boolean shouldTerminate;

    private int counterMessages;

    private final String newline = System.lineSeparator();

    // constructor
    public StompMessagingProtocolImpl() {
        shouldTerminate = false;
        counterMessages = 0;
        connections=null;
    }

    /**
     * initiate the current client protocol with it's personal connection ID and the
     * connections implementation
     */
    public void start(int connectionId, Connections<String> connections) {
        this.connectionId = connectionId;
        this.connections = (ConnectionsImpl<String>) connections;
    }

    /**
     * process the message from the client
     */
    public void process(String message) {
        if(!message.equals("") && !message.equals("\u0000") && message!=null){
        frame = message;
        recieve();
        }
    }

    /**
     * The function is used by the connection handler
     * @return true if the connection should be terminated
     */
    public boolean shouldTerminate() {
        return shouldTerminate;
    }
    
    /**
     * remove the command line from the frame
     * 
     * @return the command of the frame
     */
    private String Command() {
        String output = frame.substring(0, frame.indexOf(newline));
        frame = frame.substring(frame.indexOf(newline) + 1);
        return output;
    }

    /**
     * remove the header from the frame
     * header of '\n' means header of body frame
     * 
     * @return header of first line remaned in the frame
     */
    private String Header() {
        String output;
        if (frame.equals(""))
            return "endOfFrame";
        else if (frame.charAt(0) == '\n') { // means just body frame has remaned in the frame
            output = "\n";
            frame = frame.substring(1);
            return output; // return frame body
        } else {
            output = frame.substring(0, frame.indexOf(':'));
            frame = frame.substring(frame.indexOf(':') + 1);
            return output;
        }
    }

    /**
     * remove the value of the header from the frame
     * 
     * @return value of the header of first line remaned in the frame
     */
    private String valueOfHeader() {
        String output = frame.substring(0,frame.indexOf(newline));
        frame = frame.substring(frame.indexOf(newline) + 1);
        return output;
    }

    // ---------------------------------------------------------------------------------------
    /**
     * decode the recieved command and call the relevant function to handel with it
     */
    private void recieve() {
        String command = Command();
        if (command.equals("CONNECT"))
            CONNECT();
        else if (command.equals("SEND"))
            SEND();
        else if (command.equals("SUBSCRIBE"))
            SUBSCRIBE();
        else if (command.equals("UNSUBSCRIBE"))
            UNSUBSCRIBE();
        else if (command.equals("DISCONNECT"))
            DISCONNECT();
        else {
            String s = "the command is illigal command";
            ERROR(s, "The command "+command+" is illigal command", null); 
        }
    }

    // ---------------------------------------------------------------------------------------
    // commands that the server recieved from client

    private void CONNECT() {
        String version = null, host = null, login_userID = null, passcode = null, frame_body = null, receipt = null;
        String header = Header();

        while (header != "endOfFrame") {
            if (header.equals("accept-version")) {
                if (!valueOfHeader().equals("1.2")) {
                    ERROR("Your STOMP version is not 1.2 - can't make comunication", null, null);
                }
            } else if (header.equals("host")) {
                host = valueOfHeader(); 
            } else if (header.equals("login")) {
                login_userID = valueOfHeader();
            } else if (header.equals("passcode")) {
                passcode = valueOfHeader();
            } else if (header.equals(newline)) {
                frame = "";
            } else if (header.equals("receipt")) {
                receipt = valueOfHeader();
            }
            header = Header();
        }

        if (connections.users.containsKey(login_userID)&& connections.connectionId_connectedUser.containsValue(connections.users.get(login_userID))) {// if the user have alredy logged
                                                                                      // in - error
            ERROR("User already logged in", header, receipt);
        } else if (connections.users.containsKey(login_userID) && !connections.users.get(login_userID).passcode.equals(passcode) ) { // wrong
                                                                                                            // password
                                                                                                            // - error
            ERROR("incorrect password", "can't connect according to wrong password", receipt);
        } else if (!connections.users.containsKey(login_userID)) { // the user have not connected yet
            User u = new User(login_userID, passcode);
            connections.users.put(login_userID, u);
            connections.connectionId_connectedUser.put(connectionId, u);
        } else if (connections.users.containsKey(login_userID)
                && !connections.connectionId_connectedUser.containsValue(connections.users.get(login_userID))) { // user that was
                                                                                               // connected in the past
                                                                                               // - reconnect
                                                                                               connections.connectionId_connectedUser.put(connectionId, connections.users.get(login_userID));
        }

        CONNECTED();


        if (receipt != null) // if the CONNECT frame has receipt header, return reciept
            RECEIPT(receipt);

    }

    private void SEND() {
        String destination = null, frame_body = null, receipt = null;
        String header = Header();

        while (header != "endOfFrame") {
            if (header.equals("destination")) {
                destination = valueOfHeader();
            } else if (header.equals(newline)) {
                frame_body = frame;
                frame = "";
            } else if (header.equals("receipt")) {
                receipt = valueOfHeader();
            }
            header = Header();
        }

        if (receipt != null) { // if the CONNECT frame has receipt header, return reciept
            RECEIPT(receipt);
        }

        if (connections.topic_connectionId_subscribedId.containsKey(destination) && // the topic exists
        connections.topic_connectionId_subscribedId.get(destination).containsKey(connectionId)) { // the user subscribed
                                                                                                 // to this topic
            MESSAGE(destination, frame_body);
        } else {
            ERROR("illegal action request", "The message:" + "\n-----\n" + frame_body
                    + "\n-----\n cant get published- the user is not subscribed to the topic", receipt);
        }
    }

    private void SUBSCRIBE() {
        String destination = null, subscribedId = null, receipt = null;
        String header = Header();
        while (header != "endOfFrame") {
            if (header.equals("destination")) {
                destination = valueOfHeader();
            } else if (header.equals("id")) {
                subscribedId = valueOfHeader();
            } else if (header.equals(newline)) {
                frame = "";
            } else if (header.equals("receipt")) {
                receipt = valueOfHeader();
            }
            header = Header();
        }

        if (!connections.topic_connectionId_subscribedId.containsKey(destination)) { // if the topic is not exist yet- add it
            connections.topic_connectionId_subscribedId.put(destination, new HashMap<>());
        }
        Integer subid = Integer.valueOf(subscribedId);
        connections.topic_connectionId_subscribedId.get(destination).put(connectionId, subid); // add the suscribed id of the user to the topic
        if(!connections.subscribedId_ConnectionId_topic.containsKey(subid)){ //there is no subid like this, creat new one
            connections.subscribedId_ConnectionId_topic.put(subid, new HashMap<>());
            connections.subscribedId_ConnectionId_topic.get(subid).put(connectionId, destination);
        }
        else{
            connections.subscribedId_ConnectionId_topic.get(subid).put(connectionId, destination);
        }
        
        connections.connectionId_connectedUser.get(connectionId).subsribedIds.add(subid);
        if (receipt != null) { // if the CONNECT frame has receipt header, return reciept
            RECEIPT(receipt);
        }

    }

    private void UNSUBSCRIBE() {
        String id_subscriptionID = null, receipt = null;
        String header = Header();
        while (header != "endOfFrame") {
            if (header.equals("receipt")) {
                receipt = valueOfHeader();
            } else if (header.equals("id")) {
                id_subscriptionID = valueOfHeader();
            } else if (header.equals(newline)) {
                frame = "";
            }
            header = Header();
        }
        if (receipt != null) { // if the CONNECT frame has receipt header, return reciept
            RECEIPT(receipt);
        }
        Integer subid = Integer.valueOf(id_subscriptionID);
        if (!connections.subscribedId_ConnectionId_topic.containsKey(subid)) {
            ERROR("Wrong subscription ID", "The unsubscribed failed according to illegal subscription ID", receipt);
        } else if (!connections.subscribedId_ConnectionId_topic.get(subid).containsKey(connectionId)) {
            ERROR("Wrong subscription ID", "The unsubscribed failed according to illegal subscription ID", receipt);
        } else {
            // removing of the client from the subscription of the channel
            connections.topic_connectionId_subscribedId.get(connections.subscribedId_ConnectionId_topic.get(subid).get(connectionId))
                    .remove(connectionId);
                    connections.subscribedId_ConnectionId_topic.get(subid).remove(connectionId);
                    connections.connectionId_connectedUser.get(connectionId).subsribedIds.remove(subid);
        }

    }

    private void DISCONNECT() {
        String receipt = null;
        String header = Header();
        while (header != "endOfFrame") {
            if (header.equals("receipt")) {
                receipt = valueOfHeader();
            } else if (header.equals(newline)) {
                frame = "";
            }
            header = Header();
        }
        if (receipt != null) { // if the CONNECT frame has receipt header, return reciept
            RECEIPT(receipt);
        }
        connections.disconnect(connectionId);
        shouldTerminate = true;
    }

    // ---------------------------------------------------------------------------------------

    // commands that the server send to client

    // all the functions below assumes the frame DON'T HAVE COMMAND LINE

    private void CONNECTED() {
        String output = "CONNECTED" + "\n" + "version:1.2" + "\n"+"\n"+ "\u0000";
        connections.send(connectionId, output);
    }

    /**
     * conveys messages from a subscription to the client
     */
    private void MESSAGE(String destination, String frame_body) {
        String output;
        Map<Integer, Integer> connectionId_subid = connections.topic_connectionId_subscribedId.get(destination);
        Iterator<Map.Entry<Integer, Integer>> itr = connectionId_subid.entrySet().iterator();
        while (itr.hasNext()) {
            Map.Entry<Integer, Integer> entry = itr.next();
            output = "MESSAGE" + "\n" + "subscription:" + entry.getValue() + "\n"
                    + "message-id:" + counterMessages + "\n" + "destination:" + destination + "\n" + "\n" + frame_body
                    + "\u0000";
            counterMessages++;
            connections.send(entry.getKey(), output);
        }
    }

    private void RECEIPT(String receiptId) {
        String output;
        output = "RECEIPT" + "\n" +"receipt-id:"+ receiptId + "\n" + "\n" + "\u0000";
        connections.send(connectionId, output);
    }

    private void ERROR(String error, String bodyFrame, String receiptId) {
        String output;
        output = "ERROR"+ "\n"+error+"\n";
        if (receiptId != null){
            output +="receipt-id:"+ receiptId + "\n";
        }else if(frame.contains("receipt")){
            int indexOfBegginigOfId=frame.lastIndexOf("receipt:")+8;
            String temp=frame.substring(indexOfBegginigOfId);
            receiptId=temp.substring(0, temp.indexOf('\n'));
        }
        if (bodyFrame != null)
            output += "\n" + bodyFrame + "\n" + "\u0000";
        else {
            output += "\n" + "\u0000";
        }
        connections.send(connectionId, output);
        connections.disconnect(connectionId);
        shouldTerminate=true;
    }

}
