#include "TCPClient.h"

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#include <NetworkMessage.h>
#include <Security.h>

/**********************************************************************************************
 * TCPClient (constructor) - Creates a Stdin file descriptor to simplify handling of user input. 
 *
 **********************************************************************************************/

TCPClient::TCPClient() : Client(),
						 fd(-1),
						 selector{} {
	selector.setReadCallback([this](auto fd, const auto data, auto & buffer){onRead(fd, data, buffer);});
}

/**********************************************************************************************
 * connectTo - Opens a File Descriptor socket to the IP address and port given in the
 *             parameters using a TCP connection.
 *
 *    Throws: socket_error exception if failed. socket_error is a child class of runtime_error
 **********************************************************************************************/

void TCPClient::connectTo(const char *ip_addr, unsigned short port) {
	int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fd < 0)
		throw socket_error(std::string("failed to open client socket: ") + strerror(errno));
	
	// connect
	{
		auto bindAddr = sockaddr_in{};
		bzero(&bindAddr, sizeof(bindAddr));
		if (inet_pton(AF_INET, ip_addr, &bindAddr.sin_addr) <= 0) // returns 0 on failure as well
			throw socket_error(std::string("failed to process IP address: ") + strerror(errno));
		bindAddr.sin_family = AF_INET;
		bindAddr.sin_port = htons(port);
		if (connect(fd, reinterpret_cast<sockaddr*>(&bindAddr), sizeof(bindAddr)) < 0) {
			throw socket_error(std::string("failed to connect to server: ") + strerror(errno));
		}
	}
	
	auto flags = fcntl(fd, F_GETFL, 0);
	fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	
	this->fd = fd;
	selector.addFD(FD<void>(fd, nullptr, std::nullopt, std::nullopt, [this](auto fd) { selector.stop(); }));
	
	// Load stdin
	flags = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
	selector.addFD(FD<void>(STDIN_FILENO, nullptr, std::nullopt, [this](int fd, const BufferByte * data, size_t len){return ::write(STDOUT_FILENO, data, len);}, [](auto fd){}));
}

/**********************************************************************************************
 * handleConnection - Performs a loop that checks if the connection is still open, then 
 *                    looks for user input and sends it if available. Finally, looks for data
 *                    on the socket and sends it.
 * 
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::handleConnection() {
	fprintf(stdout, "\n----- Login -----\n\nUsername: ");
	fflush(stdout);
	clientInputState = ClientInputState::WAITING_FOR_LOGIN_USERNAME;
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
 * closeConnection - Your comments here
 *
 *    Throws: socket_error for recoverable errors, runtime_error for unrecoverable types
 **********************************************************************************************/

void TCPClient::closeConn() {
	selector.clearFDs();
}

void TCPClient::onRead(int fd, const std::shared_ptr<void> &data, DynamicBuffer & buffer) {
	if (fd != STDIN_FILENO) {
		Message message{};
		bool ready = true;
		while (ready && message.peek(buffer)) {
			switch (message.type) {
				case MessageType::DISPLAY_MESSAGE:     HANDLE_MESSAGE(onReadDisplayMessage, DisplayMessage) break;
				case MessageType::LOGIN_SET_USERNAME_RESPONSE: HANDLE_MESSAGE(onReadLoginSetUsernameResponse, LoginSetUsernameResponse) break;
				case MessageType::LOGIN_SET_PASSWORD_RESPONSE: HANDLE_MESSAGE(onReadLoginSetPasswordResponse, LoginSetPasswordResponse) break;
				case MessageType::LOGIN_AUTHENTICATE_RESPONSE: HANDLE_MESSAGE(onReadLoginAuthenticateResponse, LoginAuthenticateResponse) break;
				default:
					fprintf(stdout, "\nReceived unknown message from server: %d.\n", static_cast<int>(message.type));
					message.get(buffer);
					break;
			}
		}
		return;
	}
	while (true) {
		// Load string from input buffer
		ssize_t newline = -1;
		do {
			while (buffer.length() > 0 && (buffer[0] == '\r' || buffer[0] == '\n'))
				buffer.advanceBuffer(1);
			const auto maxLength = buffer.length();
			if (buffer.length() == 0)
				return;
			for (size_t i = 0; i < maxLength; i++) {
				if (buffer[i] == '\r' || buffer[i] == '\n') {
					newline = i;
					break;
				}
			}
		} while (newline == 0);
		
		// Copy string to something we can process
		if (newline < 0)
			return;
		assert(newline > 0);
		
		Buffer input = buffer.getNext(newline+1);
		static_assert(sizeof(int8_t) == sizeof(char), "Not a one-to-one conversion from int8_t to char");
		handleUserInput(std::string(reinterpret_cast<const char*>(input.data()), newline));
	}
}

void TCPClient::handleUserInput(std::string input) {
	std::unique_ptr<Message> message = nullptr;
	if (clientInputState == ClientInputState::WAITING_FOR_LOGIN_USERNAME) {
		message = std::make_unique<LoginSetUsername>(input);
	} else if (clientInputState == ClientInputState::WAITING_FOR_LOGIN_PASSWORD) {
		Security::INSTANCE()->setFDEcho(0, true);
		fprintf(stdout, "\n");
		message = std::make_unique<LoginAuthenticate>(input);
	} else if (clientInputState == ClientInputState::WAITING_FOR_CHANGE_PASSWORD1) {
		passwordTemporaryStorage = input;
		fprintf(stdout, "\nPlease enter your password one more time: ");
		fflush(stdout);
		clientInputState = ClientInputState::WAITING_FOR_CHANGE_PASSWORD2;
	} else if (clientInputState == ClientInputState::WAITING_FOR_CHANGE_PASSWORD2) {
		clientInputState = ClientInputState::NONE;
		Security::INSTANCE()->setFDEcho(0, true);
		if (input == passwordTemporaryStorage) {
			fprintf(stdout, "\n");
			message = std::make_unique<LoginSetPassword>(input);
		} else {
			fprintf(stdout, "\nPasswords did not match.\n");
		}
		passwordTemporaryStorage = "";
	} else if (input == "hello") {
		message = std::make_unique<HelloMessage>();
	} else if (input == "1") {
		message = std::make_unique<Generic1Message>();
	} else if (input == "2") {
		message = std::make_unique<Generic2Message>();
	} else if (input == "3") {
		message = std::make_unique<Generic3Message>();
	} else if (input == "4") {
		message = std::make_unique<Generic4Message>();
	} else if (input == "5") {
		message = std::make_unique<Generic5Message>();
	} else if (input == "passwd") {
		fprintf(stdout, "Please enter your new password: ");
		fflush(stdout);
		clientInputState = ClientInputState::WAITING_FOR_CHANGE_PASSWORD1;
		passwordTemporaryStorage = "";
		Security::INSTANCE()->setFDEcho(0, false);
		return;
	} else if (input == "menu") {
		message = std::make_unique<MenuMessage>();
	} else if (input == "exit") {
		selector.stop();
		return;
	} else {
		fprintf(stdout, "Unknown input: '%s'\n", &input[0]);
		return;
	}
	if (message != nullptr) {
		selector.writeToFD(fd, message->encode());
	}
}

void TCPClient::onReadLoginSetUsernameResponse(int fd, const std::shared_ptr<StoredDataType> &data, LoginSetUsernameResponse msg) {
	if (msg.success) {
		fprintf(stdout, "Password: ");
		fflush(stdout);
		clientInputState = ClientInputState::WAITING_FOR_LOGIN_PASSWORD;
		Security::INSTANCE()->setFDEcho(0, false);
	} else {
		selector.stop();
	}
}

void TCPClient::onReadLoginSetPasswordResponse(int fd, const std::shared_ptr<StoredDataType> &data, LoginSetPasswordResponse msg) {

}

void TCPClient::onReadLoginAuthenticateResponse(int fd, const std::shared_ptr<StoredDataType> &data, LoginAuthenticateResponse msg) {
	if (msg.success) {
		clientInputState = ClientInputState::NONE;
	} else {
		fprintf(stdout, "Password: ");
		fflush(stdout);
		clientInputState = ClientInputState::WAITING_FOR_LOGIN_PASSWORD;
		Security::INSTANCE()->setFDEcho(0, false);
	}
}

void TCPClient::onReadDisplayMessage(int fd, const std::shared_ptr<StoredDataType> &data, DisplayMessage msg) {
	write(STDOUT_FILENO, msg.message.c_str(), msg.message.length());
}
