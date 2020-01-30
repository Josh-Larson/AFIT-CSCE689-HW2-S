#pragma once

#include <string>
#include <Client.h>
#include <Selector.h>
#include <NetworkMessage.h>

class TCPClient : public Client {
	enum ClientInputState {
		NONE,
		WAITING_FOR_LOGIN_USERNAME,
		WAITING_FOR_LOGIN_PASSWORD,
		WAITING_FOR_CHANGE_PASSWORD1,
		WAITING_FOR_CHANGE_PASSWORD2
	};
	
	using StoredDataType = void;
	using StoredDataPointer = const std::shared_ptr<StoredDataType>&;
	Selector<StoredDataType> selector;
	int fd;
	ClientInputState clientInputState = ClientInputState::NONE;
	std::string passwordTemporaryStorage = "";
	
	public:
	TCPClient();
	~TCPClient() override = default;
	
	void connectTo(const char *ip_addr, unsigned short port) override;
	void handleConnection() override;
	
	void closeConn() override;
	
	private:
	void onRead(int fd, const std::shared_ptr<void>& data, DynamicBuffer & buffer);
	
	void handleUserInput(std::string input);
	void onReadLoginSetUsernameResponse(int fd, StoredDataPointer data, LoginSetUsernameResponse msg);
	void onReadLoginSetPasswordResponse(int fd, StoredDataPointer data, LoginSetPasswordResponse msg);
	void onReadLoginAuthenticateResponse(int fd, StoredDataPointer data, LoginAuthenticateResponse msg);
	static void onReadDisplayMessage(int fd, StoredDataPointer data, DisplayMessage msg);
};
