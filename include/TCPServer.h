#pragma once

#include <Server.h>
#include <Selector.h>
#include <NetworkMessage.h>
#include <Database.h>

class TCPServer : public Server {
	struct User {
		std::string username  = "";
		int passwordAttempts  = 0;
		bool usernameVerified = false;
		bool passwordVerified = false;
	};
	
	using StoredDataType = User;
	using StoredDataPointer = const std::shared_ptr<StoredDataType>&;
	Selector<StoredDataType> selector;
	Database<1, ','> whitelist {"whitelist"};
	Database<3, ','> passwd    {"passwd"};
	
	public:
	TCPServer();
	~TCPServer() override = default;
	
	void bindSvr(const char *ip_addr, unsigned short port) override;
	void listenSvr() override;
	void shutdown() final;
	
	private:
	static std::string createGreeting();
	static std::string createMenu();
	
	void onRead(int fd, StoredDataPointer data, DynamicBuffer & buffer);
	
	void onReadHelloRequest(int fd, StoredDataPointer data, HelloMessage msg);
	void onReadGeneric1Request(int fd, StoredDataPointer data, Generic1Message msg);
	void onReadGeneric2Request(int fd, StoredDataPointer data, Generic2Message msg);
	void onReadGeneric3Request(int fd, StoredDataPointer data, Generic3Message msg);
	void onReadGeneric4Request(int fd, StoredDataPointer data, Generic4Message msg);
	void onReadGeneric5Request(int fd, StoredDataPointer data, Generic5Message msg);
	void onReadMenuRequest(int fd, StoredDataPointer data, MenuMessage msg);
	void onReadLoginSetUsername(int fd, StoredDataPointer data, LoginSetUsername msg);
	void onReadLoginSetPassword(int fd, StoredDataPointer data, LoginSetPassword msg);
	void onReadLoginAuthenticate(int fd, StoredDataPointer data, LoginAuthenticate msg);
};
