cmake_minimum_required(VERSION 3.10.2)
project(AFIT-CSCE689-HW2)

set(CMAKE_CXX_STANDARD 17)

add_executable(adduser src/adduser_main.cpp
               src/Database.cpp include/Database.h
               src/Security.cpp include/Security.h)
add_executable(Client src/client_main.cpp src/strfuncts.cpp include/strfuncts.h
               src/Client.cpp include/Client.h
               src/TCPClient.cpp include/TCPClient.h
               src/Security.cpp include/Security.h
               src/Selector.cpp include/Selector.h
               src/NetworkMessage.cpp include/NetworkMessage.h
               include/exceptions.h include/strfuncts.h)
add_executable(Server src/server_main.cpp src/strfuncts.cpp include/strfuncts.h
               src/Server.cpp include/Server.h
               src/TCPServer.cpp include/TCPServer.h
               src/Security.cpp include/Security.h
               src/Database.cpp include/Database.h
               src/Selector.cpp include/Selector.h
               src/NetworkMessage.cpp include/NetworkMessage.h
               include/exceptions.h include/strfuncts.h)

target_include_directories(adduser PRIVATE src include)
target_include_directories(Client PRIVATE src include)
target_include_directories(Server PRIVATE src include)

target_link_libraries(adduser argon2)
target_link_libraries(Client argon2)
target_link_libraries(Server argon2)
