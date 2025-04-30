# Subscription-Server

a ”community-led” world cup update subscription
service for the soccer world cup, utilizing STOMP and supporting both Thread Per Client (TPC) and Reactor. 

## Client commands:

For any command below requiring a game_name input: game_name for a game
between some Team A and some Team B should always be of the form
<team_a_name>_<Team_b_name>

### Login command
○ Structure: login {host:port} {username} {password}

○ For this command a CONNECT frame is sent to the server.

Possible outputs:

∎ Socket error: connection error. In this case, the output should
be ”Could not connect to server”.

∎ Client already logged in: If the client has already logged into a
server you should not attempt to log in again. The client should
simply print ”The client is already logged in, log out before trying
again”.

∎ New user: If the server connection was successful and the server
doesn’t find the username, then a new user is created, and the
password is saved for that user. Then the server sends a
CONNECTED frame to the client and the client will print ”Login
successful”.

∎ User is already logged in: If the user is already logged in, then
the server will respond with a STOMP error frame indicating
the reason – the output, in this case, should be ”User already
logged in”.

∎ Wrong password: If the user exists and the password doesn’t
match the saved password, the server will send back an appropriate ERROR frame indicating the reason - the output, in this
case, should be ”Wrong password”.

∎ User exists: If the server connection was successful, the server
will check if the user exists in the users’ list and if the password
matches, also the server will check that the user does not have an
active connection already. In case these tests are OK, the server
sends back a CONNECTED frame and the client will print to
the screen ”Login successful”.

Example:

○ Command: login 1.1.1.1:2000 meni films

### Join Game Channel command

○ Structure: join {game_name}

○ For this command a SUBSCRIBE frame is sent to the {game_name} topic.

○ As a result, a RECIEPT will be returned to the client. A message
”Joined channel {game_name}” will be displayed on the screen.

○ From now on, any message received by the client from
{game_name} will be parsed and used to update the information of
the game as specified in the game events section. Each report will contain the name of
the reporter, and you reports from different users will be saved separately.

Example:

○ Command: join germany_spain

### Exit Game Channel command

○ Structure: exit {game_name}

○ For this command an UNSUBSCRIBE frame is sent to the {game_name}
topic.

○ As a result, a RECIEPT will be returned to the client. A message
”Exited channel {game_name}” will be displayed on the screen.

Example:

○ Command: exit germany_spain

### Report to channel command

○ Structure: report {file}

○ For this command, the client will do the following:

1. Read the provided {file} and parse the game name and events
it contains (more on the file format in the game event section).

2. Save each event on the client as a game update reported by the
current logged-in user. You should save the events ordered by
the time specified in them, as you will need to summarize them
in that order in the summary command.

3. Send a {SEND} frame for each game event to the {game_name}
topic (which, as mentioned, should be parsed from within the
file), containing all the information of the game event in its body,
as well as the name of the {user}.

An example of a SEND frame containing a report:

SEND
destination :/ spain_japan
user : meni
team a : spain
team b : japan
event name : kickoff
time : 0
general game updates :
active : true
before halftime : true
team a updates :
a : b
c : d
e : f
team b updates :
a : b
c : d
e : f
description :
And we ’ re off !
^ @

You should format the body of your reports as in the example
above.

Example: (with the events1_partial.json file)
○ Command: report events1_partial.json

The game event reports will be printed in the order that they
happened in the game, and the stats will be printed ordered lexicographically by their name.

○ If {file} doesn’t exist, it will create it. Otherwise, it will write over its content.

○ Note that {user} can be the clients current active user. This should
not cause a problem for this command since the client is saving every
game event it sends.

### Logout Command

○ Structure: logout

○ This command tells the client that the user wants to log out from
the server. The client will send a DISCONNECT to the server.

○ The server will reply with a RECEIPT frame.

○ The logout command removes the current user from all the topics.

○ Once the client receives the RECEIPT frame, it should close the socket
and await further user commands.

Example:

○ Command: logout
