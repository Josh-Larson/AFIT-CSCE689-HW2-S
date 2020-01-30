bin_PROGRAMS = tcpserver tcpclient my_adduser


tcpserver_SOURCES = server_main.cpp Server.cpp TCPServer.cpp Security.cpp Selector.cpp Database.cpp NetworkMessage.cpp
tcpserver_LDFLAGS = -largon2

tcpclient_SOURCES = client_main.cpp Client.cpp TCPClient.cpp Security.cpp Selector.cpp Database.cpp NetworkMessage.cpp
tcpclient_LDFLAGS = -largon2

my_adduser_SOURCES = adduser_main.cpp Security.cpp Database.cpp
my_adduser_LDFLAGS = -largon2
