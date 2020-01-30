#include "TCPServer.h"

#include <exceptions.h>
#include <NetworkMessage.h>
#include <Database.h>
#include <Security.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <cstring>

TCPServer::TCPServer() : Server(),
						 selector{} {
	selector.setReadCallback([this](auto fd, const auto & data, auto & buffer){onRead(fd, data, buffer);});
	selector.setCloseCallback([this](auto fd, const auto & data){onClose(fd, data);});
}

/**********************************************************************************************
 * bindSvr - Creates a network socket and sets it nonblocking so we can loop through looking for
 *           data. Then binds it to the ip address and port
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::bindSvr(const char *ip_addr, short unsigned int port) {
	int fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, IPPROTO_TCP);
	if (fd < 0)
		throw socket_error(std::string("failed to open server socket: ") + strerror(errno));
	
	int one = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(int));
	
	// bind
	{
		auto bindAddr = sockaddr_in{};
		bzero(&bindAddr, sizeof(bindAddr));
		if (inet_pton(AF_INET, ip_addr, &bindAddr.sin_addr) <= 0) // returns 0 on failure as well
			throw socket_error(std::string("failed to process IP address: ") + strerror(errno));
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(port);
		if (bind(fd, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) < 0)
			throw socket_error(std::string("failed to bind server socket: ") + strerror(errno));
	}
	
	// listen
	if (listen(fd, 32) < 0)
		throw socket_error(std::string("failed to listen on server socket: ") + strerror(errno));
	
	selector.addFD(FD<StoredDataType>(fd, nullptr, [&](int fd) -> std::shared_ptr<Buffer>{
		auto bindAddr = sockaddr_storage{};
		auto bindAddrLength = socklen_t{sizeof(bindAddr)};
		auto accepted = accept4(fd, reinterpret_cast<sockaddr*>(&bindAddr), &bindAddrLength, SOCK_NONBLOCK);
		if (accepted >= 0) {
			std::array<char, 256> addr{};
			const char * data;
			if (bindAddr.ss_family == AF_INET)
				data = inet_ntop(bindAddr.ss_family, &(((sockaddr_in*)&bindAddr)->sin_addr), addr.data(), addr.size());
			else
				data = inet_ntop(bindAddr.ss_family, &(((sockaddr_in6*)&bindAddr)->sin6_addr), addr.data(), addr.size());
			if (data == nullptr) {
				fprintf(stderr, "Failed to parse IP address: %s\n", strerror(errno));
				close(accepted);
				return nullptr;
			}
			std::string ip = data;
			if (!whitelist.find([ip, data=std::string(data)](const auto & row) -> bool { return row[0] == ip; })) {
				fprintf(stdout, "Unrecognized client IP: %s\n", data);
				log("Unrecognized client IP: " + ip);
				close(accepted);
				return nullptr;
			}
			fprintf(stdout, "Received connection %d from %s\n", accepted, data);
			log("Received connection from " + ip);
			selector.addFD(FD<StoredDataType>(accepted, std::make_shared<StoredDataType>(StoredDataType {.ip = ip})));
		}
		return nullptr;
	}, /* writeHandler */ [](auto, auto, auto){return -1;}, /* closeHandler */ [](auto fd){close(fd);}));
	
	log("Server Started.");
}

/**********************************************************************************************
 * listenSvr - Performs a loop to look for connections and create TCPConn objects to handle
 *             them. Also loops through the list of connections and handles data received and
 *             sending of data. 
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::listenSvr() {
	auto code = selector.selectLoop();
	switch (code) {
		case SelectLoopTermination::SUCCESS:
		default:
			fprintf(stdout, "\nShutting down...\n");
			break;
		case SelectLoopTermination::INTERRUPTED:
			fprintf(stdout, "\nReceived signal requesting shutdown. Shutting down...\n");
			break;
		case SelectLoopTermination::SOCKET_ERROR:
			fprintf(stdout, "\nUnknown socket error: %s\n", strerror(errno));
			break;
	}
}

/**********************************************************************************************
 * shutdown - Cleanly closes the socket FD.
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPServer::shutdown() {
	selector.clearFDs();
}

void TCPServer::onClose(int fd, const std::shared_ptr<StoredDataType> &data) {
	log(data->username + " disconnected from " + data->ip);
}

void TCPServer::onRead(int fd, StoredDataPointer data, DynamicBuffer & buffer) {
	Message message{};
	bool ready = true;
	while (ready && message.peek(buffer)) {
		fprintf(stdout, "Received message: %d\n", static_cast<int>(message.type));
		switch (message.type) {
			case MessageType::HELLO:     HANDLE_MESSAGE(onReadHelloRequest,    HelloMessage) break;
			case MessageType::GENERIC_1: HANDLE_MESSAGE(onReadGeneric1Request, Generic1Message) break;
			case MessageType::GENERIC_2: HANDLE_MESSAGE(onReadGeneric2Request, Generic2Message) break;
			case MessageType::GENERIC_3: HANDLE_MESSAGE(onReadGeneric3Request, Generic3Message) break;
			case MessageType::GENERIC_4: HANDLE_MESSAGE(onReadGeneric4Request, Generic4Message) break;
			case MessageType::GENERIC_5: HANDLE_MESSAGE(onReadGeneric5Request, Generic5Message) break;
			case MessageType::MENU:      HANDLE_MESSAGE(onReadMenuRequest,     MenuMessage) break;
			case MessageType::DISPLAY_MESSAGE: break; // You're not the boss of me!
			case MessageType::LOGIN_SET_USERNAME: HANDLE_MESSAGE(onReadLoginSetUsername, LoginSetUsername) break;
			case MessageType::LOGIN_SET_PASSWORD: HANDLE_MESSAGE(onReadLoginSetPassword, LoginSetPassword) break;
			case MessageType::LOGIN_AUTHENTICATE: HANDLE_MESSAGE(onReadLoginAuthenticate, LoginAuthenticate) break;
			case MessageType::UNKNOWN:
			default:
				selector.writeToFD(fd, std::make_shared<Buffer>("Unknown message!\n"));
				fprintf(stdout, "Unknown message!\n");
				message.get(buffer);
				break;
		}
//		buffer.getNext(&message, sizeof(Message));
	}
}

std::string TCPServer::createGreeting() {
	return "Welcome!\n\n" + createMenu();
}

std::string TCPServer::createMenu() {
	return "Available Commands:\n"
		   "    hello      Custom server greeting\n"
		   "    1,2,3,4,5  Each number provides a different message\n"
	 	   "    passwd     In a future version, this will allow you to change your password\n"
	 	   "    menu       Displays this menu\n"
		   "    exit       Disconnects you from the server\n";
}

void TCPServer::onReadHelloRequest(int fd, const std::shared_ptr<StoredDataType> &data, HelloMessage msg) {
	selector.writeToFD(fd, DisplayMessage("Hello there.\n").encode());
}

void TCPServer::onReadGeneric1Request(int fd, const std::shared_ptr<StoredDataType> &data, Generic1Message msg) {
	selector.writeToFD(fd, DisplayMessage("So uncivilized\n").encode());
}

void TCPServer::onReadGeneric2Request(int fd, const std::shared_ptr<StoredDataType> &data, Generic2Message msg) {
	selector.writeToFD(fd, DisplayMessage("I don't like sand. It's coarse and rough and irritating... and it gets everywhere\n").encode());
}

void TCPServer::onReadGeneric3Request(int fd, const std::shared_ptr<StoredDataType> &data, Generic3Message msg) {
	selector.writeToFD(fd, DisplayMessage("Now this is podracing\n").encode());
}

void TCPServer::onReadGeneric4Request(int fd, const std::shared_ptr<StoredDataType> &data, Generic4Message msg) {
	selector.writeToFD(fd, DisplayMessage("I AM the Senate.\n").encode());
}

void TCPServer::onReadGeneric5Request(int fd, const std::shared_ptr<StoredDataType> &data, Generic5Message msg) {
	selector.writeToFD(fd, DisplayMessage("*kills younglings*\n").encode());
}

void TCPServer::onReadMenuRequest(int fd, const std::shared_ptr<StoredDataType> &data, MenuMessage msg) {
	selector.writeToFD(fd, DisplayMessage(createMenu()).encode());
}

void TCPServer::onReadLoginSetUsername(int fd, const std::shared_ptr<StoredDataType> &data, LoginSetUsername msg) {
	if (data->usernameVerified) {
		selector.writeToFD(fd, DisplayMessage("You are already logged in!\n").encode());
		return;
	}
	if (passwd.find([&](const auto & row) { return row[0] == msg.username; })) {
		data->username = msg.username;
		data->usernameVerified = true;
		selector.writeToFD(fd, DisplayMessage("Welcome to the server, " + msg.username + "\n").encode());
		selector.writeToFD(fd, LoginSetUsernameResponse(true).encode());
	} else {
		log("Unknown username: " + msg.username + " from " + data->ip);
		selector.writeToFD(fd, LoginSetUsernameResponse(false).encode());
		selector.removeFD(fd);
	}
}

void TCPServer::onReadLoginSetPassword(int fd, const std::shared_ptr<StoredDataType> &data, LoginSetPassword msg) {
	if (!data->usernameVerified || !data->passwordVerified) {
		selector.writeToFD(fd, DisplayMessage("You are not logged in!\n").encode());
		selector.removeFD(fd);
		return;
	}
	bool updated = false;
	bool success = passwd.update([&](const auto & row) -> Database<3, ','>::DatabaseRow {
		if (row[0] == data->username) {
			updated = true;
			return {row[0], row[1], Security::INSTANCE()->hash(msg.password, row[1])};
		}
		return row;
	});
	if (success && updated) {
		selector.writeToFD(fd, DisplayMessage("Password Changed.\n").encode());
		selector.writeToFD(fd, LoginSetPasswordResponse(true).encode());
	} else {
		selector.writeToFD(fd, DisplayMessage("Failed to update your password.\n").encode());
		selector.writeToFD(fd, LoginSetPasswordResponse(false).encode());
		// TODO: Handle user disappearing after logging in?
	}
}

void TCPServer::onReadLoginAuthenticate(int fd, const std::shared_ptr<StoredDataType> &data, LoginAuthenticate msg) {
	if (!data->usernameVerified) {
		selector.writeToFD(fd, DisplayMessage("You are not logged in!\n").encode());
		selector.removeFD(fd);
		return;
	}
	auto userData = passwd.find([&](const auto & row) { return row[0] == data->username; });
	if (!userData) {
		selector.writeToFD(fd, DisplayMessage("Your username disappeared.\n").encode());
		selector.writeToFD(fd, LoginAuthenticateResponse(false).encode());
		selector.removeFD(fd);
		return;
	}
	auto hashed = Security::INSTANCE()->hash(msg.password, (*userData)[1]);
	data->passwordAttempts++;
	if (hashed == (*userData)[2]) {
		data->passwordVerified = true;
		selector.writeToFD(fd, DisplayMessage(createGreeting()).encode());
		selector.writeToFD(fd, LoginAuthenticateResponse(true).encode());
		log(data->username + " successfully logged in from " + data->ip);
	} else {
		selector.writeToFD(fd, DisplayMessage("Invalid password.  "+std::to_string(3-data->passwordAttempts)+" attempt"+(data->passwordAttempts==2 ? "" : "s")+" remaining.\n").encode());
		selector.writeToFD(fd, LoginAuthenticateResponse(false).encode());
		if (data->passwordAttempts >= 3) {
			selector.removeFD(fd);
		} else if (data->passwordAttempts >= 2) {
			log("Two failed password attempts from "+data->username+" at "+data->ip);
		}
	}
}

void TCPServer::log(std::string data) {
	auto result = time(nullptr);
	std::array<char, 100> timeString{};
	std::strftime(timeString.data(), timeString.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&result));
	logfile.insert({timeString.data(), std::move(data)});
}
