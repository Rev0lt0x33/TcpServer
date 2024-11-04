//
// Created by Rev0lt on 19.10.2024.
//

#ifndef TCPSERVER_H
#define TCPSERVER_H
#include <winsock2.h>
#include <iostream>
#include <ws2tcpip.h>
#include <map>
#include <thread>
#include <unordered_map>
#include <algorithm>

enum class CommandType {
    WHO,
    NAME,
    LIST,
    MSG,
    QUIT,
    ERR,
    INVALID,
    MYID
 };

class TcpServer {
public:
    void start(LPCSTR serverIp,int port);

private:
    sockaddr_in _serverAddress = {0};
    std::map<std::string, SOCKET> _clients;
    std::mutex _clientMutex;
    SOCKET _serverSocket = INVALID_SOCKET;
    bool isRunning = FALSE;

    void handleClient(SOCKET clientSocket);
    void cleanUp();
    void listenToClient();
    void sendMessage(SOCKET clientSocket,const std::string& message);
    void processCommand(SOCKET clientSocket,const std::string& command);
    std::string getCommandKey(const std::string &command);
    void removeClient(SOCKET clientSocket);

    CommandType getCommandType(const std::string& commandKey);
    bool cmd_NAME(SOCKET clientSocket,const std::string& command);
    void cmd_LIST(SOCKET clientSocket);
    void cmd_CERROR(const std::string& command);
    void cmd_MSG(SOCKET clientSocket, const std::string& command);
    void cmd_QUIT(SOCKET clientSocket);
    void cmd_ID(SOCKET clientSocket);

};



#endif //TCPSERVER_H
