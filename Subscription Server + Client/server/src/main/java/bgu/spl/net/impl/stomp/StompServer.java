package bgu.spl.net.impl.stomp;

import java.util.*;
import java.io.*;
import bgu.spl.net.srv.ConnectionHandler;
import bgu.spl.net.srv.ConnectionsImpl;
import bgu.spl.net.srv.Server;
import bgu.spl.net.api.MessageEncoderDecoder;
import bgu.spl.net.api.MessagingProtocol;
import bgu.spl.net.api.StompMessagingProtocol;
import bgu.spl.net.api.MessagingProtocol;

import java.net.ServerSocket;
import java.util.function.Supplier;

public class StompServer<T>{

    public static void main(String[] args) {
        int port = Integer.parseInt(args[0]);
        
        
        if (args[1].equals("tpc")) {
            
            Server.threadPerClient(port, () -> new StompMessagingProtocolImpl(), StompSrvEncDec::new).serve();
        } else {
            Server.reactor(Runtime.getRuntime().availableProcessors(), port, () -> new StompMessagingProtocolImpl(), StompSrvEncDec::new).serve();

        }
    }

}
