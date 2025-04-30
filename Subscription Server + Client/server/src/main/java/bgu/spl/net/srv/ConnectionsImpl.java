package bgu.spl.net.srv;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Handler;

import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.impl.stomp.StompMessagingProtocolImpl;
import bgu.spl.net.impl.stomp.User;

public class ConnectionsImpl<T> implements Connections<T> {

    public Map<Integer, ConnectionHandler<T>> connectionId_connectionHandler;


    public Map<String, Map<Integer, Integer>> topic_connectionId_subscribedId; // key- topic,
    // value- Map of key- connectionId , value- the user subscribed ID

   /**
    * map of the current connected users 
    */
    public Map<Integer, User> connectionId_connectedUser; // key- userId, value- user
    
    /**
     * map of all the users that connected the system 
     */
    public Map<String, User> users; // key- userId, value- user

    public Map<Integer, Map<Integer, String>> subscribedId_ConnectionId_topic; // key- subscribed Id, value- connection
                                                                               // Id
    private int idConnectionsCounter;

    public ConnectionsImpl() {
        connectionId_connectionHandler = new ConcurrentHashMap<>();
        idConnectionsCounter=0;
        topic_connectionId_subscribedId = new ConcurrentHashMap<>();
        connectionId_connectedUser = new ConcurrentHashMap<>();
        users = new HashMap<>();
        subscribedId_ConnectionId_topic = new ConcurrentHashMap<>();
    }

    public boolean send(int connectionId, T msg) {
        ConnectionHandler<T> cHandler = connectionId_connectionHandler.get(connectionId);
        if (cHandler != null) {
            synchronized (cHandler)
             {
                cHandler.send(msg);
            }
            return true;
        } else {
            return false;
        }
    }

    public void send(String channel, T msg) {
        // we chose not to implement this function
    }

    public void disconnect(int connectionId) {
        // remove user from all subscription of topics
        User u = connectionId_connectedUser.get(connectionId);
        ConnectionHandler<T> cHandler = connectionId_connectionHandler.get(connectionId);
        if (cHandler != null && u!=null) {
            for (int i = 0; i < u.subsribedIds.size(); i++) {
                int subid = u.subsribedIds.get(i);
                String topic = subscribedId_ConnectionId_topic.get(subid).get(connectionId);
                topic_connectionId_subscribedId.get(topic).remove(connectionId);
                subscribedId_ConnectionId_topic.get(subid).remove(connectionId);
            }
            u.subsribedIds.clear();

            // remove the user from connecting users
            connectionId_connectedUser.remove(connectionId);

            // close the handler
            try {
                if (cHandler != null) {
                    synchronized (cHandler)
                     {
                        cHandler.close();
                    }
                }
            } catch (Exception ignored) {};
        }
    }

    public int addConnection(ConnectionHandler <T> handler){
        connectionId_connectionHandler.put(idConnectionsCounter, handler);
        idConnectionsCounter++;
        return idConnectionsCounter-1;
    }

}