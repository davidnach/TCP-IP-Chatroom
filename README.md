# TCP-IP-Chatroom
   Server and client code for Multiuser chatroom implemented using TCP/IP client-server model. Server supports up to 255 concurrent users and allows for private and public messages between users. A projects from WWU's Networks class. *temporarily public*

# Building
    gcc -c prog3_server.c
    gcc -c prog3_observer.c
    gcc -c prog3_participant.c

# Usage
    ./prog3_server participant_port observer_port
    
    ./prog3_participant server_address participant_port
    
    ./prog3_observer server_address observer_port
   
# Description 
    Program: prog3_observer.c
            Purpose: allocate a socket, connect to the server, connect to a participant to print private or public messages associated with that       participant.
    Program: prog3_server.c
            purpose: establish two ports, two listening sockets, accept and handle connections, host a chatroom.
    Program: prog3_participant.c
            purpose: allocate a socket, connect to the server, send public or private messages in the chatroom.
    
    server_address - name of a computer on which server is executing
    server_port    - protocol port number server is using
    participant_port - port on which chatters connect
    observer_port    - port on which observers connect
