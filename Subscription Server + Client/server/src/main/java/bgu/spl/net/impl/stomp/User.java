package bgu.spl.net.impl.stomp;

import java.util.LinkedList;
import java.util.List;

public class User {
    public String userId;
    public String passcode;
    public boolean isConnected;
    public List <Integer> subsribedIds;

    public User(String userId, String passcode){
        this.userId=userId;
        this.passcode=passcode;
        isConnected=true;
        subsribedIds=new LinkedList<>();
    }
    
}
