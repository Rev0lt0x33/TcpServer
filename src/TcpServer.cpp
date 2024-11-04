//include_directories(src)
//include_directories(include)
// Created by Rev0lt on 19.10.2024.
//

#include "../include/TcpServer.h"



void TcpServer::start(LPCSTR ip,int port){

	WSADATA wsaData;
	int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (result != 0) {
		std::cerr << "WSAStartup failed: " << result << std::endl;
		return;
	}

	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_serverSocket == INVALID_SOCKET) {
		std::cerr << "Creting socket failed: " << WSAGetLastError() << std::endl;
		WSACleanup();
		return;
	}
	_serverAddress.sin_family = AF_INET;
	_serverAddress.sin_port = htons(port);  // Server port
	inet_pton(AF_INET, ip, &_serverAddress.sin_addr);  // Bind to any available network interface

	// Bind the socket
	result = bind(_serverSocket, (sockaddr*)&_serverAddress, sizeof(_serverAddress));
	if (result == SOCKET_ERROR) {
		std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
		cleanUp();
		return;
	}

	listenToClient();
}

void TcpServer::cleanUp() {
	if(_serverSocket == INVALID_SOCKET) return;
	closesocket(_serverSocket);
	WSACleanup();
}



void TcpServer::listenToClient() {
	int result = listen(_serverSocket, SOMAXCONN) ;
	sockaddr_in clientAddress = {0};
	int clientAddrSize = sizeof(clientAddress);

	if (result == SOCKET_ERROR) {
		std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
		cleanUp();
		return;
	}

	std::cout << "Waiting for client connections..." << std::endl;
	isRunning = TRUE;

	while(isRunning == TRUE) {
		clientAddress = {0};
		SOCKET clientSocket = accept(_serverSocket,(sockaddr*)&clientAddress,&clientAddrSize);

		if(clientSocket == INVALID_SOCKET) {
			int errorCode = WSAGetLastError();
			std::cerr << "Accept failed, error: " << errorCode << std::endl;

			if(errorCode == WSAETIMEDOUT ||  errorCode == WSAECONNRESET) {
				std::cerr << "Error not critical. Continuing listening." << std::endl;
				continue;
			}
			std::cerr << "Critical error. Shutting down." <<std::endl;
			cleanUp();
			break;
		}

		char clientIP[INET_ADDRSTRLEN] = {0};
		inet_ntop(AF_INET, &clientAddress.sin_addr, clientIP, INET_ADDRSTRLEN);
		std::cout << "Client connecting from IP: " << clientIP << " and port: " << ntohs(clientAddress.sin_port) << std::endl;

		std::thread(&TcpServer::handleClient, this, clientSocket).detach();

	}
}



void TcpServer::handleClient(SOCKET clientSocket) {
	char buffer[512];
	std::string clientCommand;
	int bytesReceived;
	sendMessage(clientSocket,"WHO");

	while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0)) > 0) {
		buffer[bytesReceived] = '\0';
		clientCommand = buffer;

		if (getCommandKey(clientCommand) == "QUIT") {
			cmd_QUIT(clientSocket);
			break;
		}
		if(getCommandKey(clientCommand) == "NAME" ) {
			bool res = cmd_NAME(clientSocket,clientCommand);
			if(res == FALSE ) break;

		}else {
			processCommand(clientSocket, clientCommand);
		}

	}
	//TODO: delet client when 10054 - conn reset
	if (bytesReceived == SOCKET_ERROR) {
		int err = WSAGetLastError();
		if(err == 10054) {
			removeClient(clientSocket);
			std::cout << "Client disconnected."<< std:: endl;
		}else {
			std::cerr << "recv failed: " << err  << std::endl;
		}

	}
	std::cout << "" << std::endl;
}

void TcpServer::sendMessage(SOCKET clientSocket,const std::string& message) {
	send(clientSocket,message.c_str(),message.length(),0);
}
CommandType TcpServer::getCommandType(const std::string& commandKey) {
	const std::unordered_map<std::string, CommandType> commandMap = {
		{"WHO", CommandType::WHO},
		{"NAME", CommandType::NAME},
		{"LIST", CommandType::LIST},
		{"MSG", CommandType::MSG},
		{"QUIT", CommandType::QUIT},
		{"ERROR", CommandType::ERR},
		 {"MYID", CommandType::MYID}
	};

	auto it = commandMap.find(commandKey);
	if (it != commandMap.end()) {
		return it->second;
	}
	return CommandType::INVALID;
}
void TcpServer::processCommand(SOCKET clientSocket, const std::string& command) {
	std::string commandKey = getCommandKey(command);
	CommandType cmdType = getCommandType(commandKey);
	std::cout << command << std::endl;
	switch (cmdType) {
		case CommandType::LIST:
			cmd_LIST(clientSocket);
			break;
		case CommandType::MSG:
			cmd_MSG(clientSocket,command);
			break;
		case CommandType::QUIT:
			cmd_QUIT(clientSocket);
			break;
		case CommandType::ERR:
			cmd_CERROR(command);
			break;
		case CommandType::MYID:
			cmd_ID(clientSocket);
			break;
		default:
			sendMessage(clientSocket, "ERROR: Invalid command.");
			break;
	}
}

std::string TcpServer::getCommandKey(const std::string &command) {
	size_t pos = command.find(" ");
	if (pos == std::string::npos) {
		return command;
	}
	return command.substr(0,pos);
}

bool TcpServer::cmd_NAME(SOCKET clientSocket,const std::string& command) {
	int posA = command.find("<");
	int posB = command.find(">");

	if(posB-posA < 8) {
		sendMessage(clientSocket, "ERROR ID to short.");
		closesocket(clientSocket);
		return FALSE;
	}

	std::string clientId = command.substr(posA+1,posB-posA-1);
	std::lock_guard<std::mutex> lock(_clientMutex);

	for(const auto& pair : _clients) {
		if(pair.first == clientId) {
			sendMessage(clientSocket, "ERROR ID already connected.");
			closesocket(clientSocket);
			return FALSE;
		}
	}
	_clients[clientId] = clientSocket;
	sendMessage(clientSocket, "ACCEPT Client connected to server.");
	return TRUE;
}

void TcpServer::cmd_LIST(SOCKET clientSocket) {
	std::lock_guard<std::mutex> lock(_clientMutex);
	std::string list = "Clients:";

	for(const auto& pair: _clients) {
		list += pair.first + ", ";
	}
	sendMessage(clientSocket,list);
}

void TcpServer::cmd_CERROR(const std::string &command) {
	std::cerr << "Client could not handle command: " << command << std::endl;
}

void TcpServer::cmd_MSG(SOCKET clientSocket,const std::string &command) {
	size_t pos = command.find("<");
	size_t pos2 = command.find(" ", pos);
	std::string msg = command.substr(pos2+1);
	std::string clientId;
	std::string sendToId = command.substr(pos+1,pos2-pos-2);

	if(pos2-pos-2 != 8) {
		sendMessage(clientSocket, "Invalid user id. " + sendToId);
	}

	std::lock_guard<std::mutex> lock(_clientMutex);
	for (const auto& pair : _clients) {
		if (pair.second == clientSocket) {
			clientId = pair.first;
		}
	}
	for (const auto& pair : _clients) {
		if (pair.first == sendToId) {
			sendMessage(pair.second, "MESG from " + clientId + ": " + msg);
			sendMessage(clientSocket,"MESG sent");
			return;
		}
	}
	sendMessage(clientSocket,"User with id " + sendToId + " not connected");
}
void TcpServer::cmd_QUIT(SOCKET clientSocket) {
	sendMessage(clientSocket,"Disconnected from server.");
	closesocket(clientSocket);
	removeClient(clientSocket);
}

void TcpServer::removeClient(SOCKET clientSocket) {
	std::lock_guard<std::mutex> lock(_clientMutex);

	auto it = std::find_if(_clients.begin(), _clients.end(),
		[clientSocket](const std::pair<std::string, SOCKET>& pair) {
			return pair.second == clientSocket;
		});

	if (it != _clients.end()) {
		std::cout << "Client removed: " << it->first << std::endl;
		_clients.erase(it);
	}else {
		std::cout << "Client not found." << std::endl;
	}
}

void TcpServer::cmd_ID(SOCKET clientSocket) {
	std::lock_guard<std::mutex> lock(_clientMutex);
	for (const auto& pair : _clients) {
		if (pair.second == clientSocket) {
			sendMessage(clientSocket,"Yours id:" + pair.first);
		}
	}
}


