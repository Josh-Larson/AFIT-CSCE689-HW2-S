#pragma once

#include <Selector.h>

#include <cstdint>
#include <cassert>
#include <memory>
#include <netinet/in.h>

enum class MessageType : uint8_t {
	UNKNOWN     = 0,
	HELLO       = 1,
	GENERIC_1   = 2,
	GENERIC_2   = 3,
	GENERIC_3   = 4,
	GENERIC_4   = 5,
	GENERIC_5   = 6,
	MENU        = 7,
	DISPLAY_MESSAGE             = 8,
	LOGIN_SET_USERNAME          = 9,
	LOGIN_SET_USERNAME_RESPONSE = 10,
	LOGIN_SET_PASSWORD          = 11,
	LOGIN_SET_PASSWORD_RESPONSE = 12,
	LOGIN_AUTHENTICATE          = 13,
	LOGIN_AUTHENTICATE_RESPONSE = 14
};

/* Not ideal.. but works for the time being */
#define HANDLE_MESSAGE(FUNCTION, MESSAGE_TYPE)  {\
                                                    MESSAGE_TYPE msg;\
                                                    if (msg.get(buffer)) \
                                                        FUNCTION(fd, data, msg); \
                                                    else \
                                                    	ready = false; \
                                                }

struct Message {
	uint16_t    size = 0;
	MessageType type = MessageType::UNKNOWN;
	
	Message() = default;
	Message(uint16_t size, MessageType type) : size(size), type(type) {}
	virtual ~Message() = default;
	
	virtual bool peek(const DynamicBuffer & buffer) {
		if (buffer.length() < 3)
			return false;
		const auto buf = buffer.getNextBuffer();
		peekHeader(buf);
		return buf->length() >= size;
	}
	
	virtual bool get(DynamicBuffer & buffer) {
		bool success = peek(buffer);
		if (success)
			buffer.advanceBuffer(size);
		return success;
	}
	
	virtual std::shared_ptr<Buffer> encode() {
		std::array<BufferByte, 3> encoded {};
		*reinterpret_cast<uint16_t*>(encoded.data()) = htons(size);
		encoded[2] = static_cast<uint8_t>(type);
		return std::make_shared<Buffer>(encoded.data(), 3);
	}
	
	void peekHeader(const std::shared_ptr<Buffer> & buf) {
		size = ntohs(*reinterpret_cast<const uint16_t*>(buf->data()));
		type = static_cast<MessageType>(buf->get(2));
	}
	
	static std::optional<std::string> peekString(const DynamicBuffer & buffer, size_t index) {
		uint16_t length = ntohs(static_cast<uint16_t>(static_cast<uint16_t>(buffer[index+1]) << 8u) | static_cast<uint16_t>(buffer[index]));
		if (length > buffer.length() - index - 2)
			return std::nullopt;
		std::string ret(length, ' ');
		for (size_t i = index+2, retIndex = 0; i < index+length; i++, retIndex++) {
			ret[retIndex] = buffer[i];
		}
		return ret;
	}
};

struct HelloMessage : public Message {
	HelloMessage() : Message(3, MessageType::HELLO) {}
	~HelloMessage() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::HELLO);
		return size == 3 && type == MessageType::HELLO;
	}
};

struct Generic1Message : public Message {
	Generic1Message() : Message(3, MessageType::GENERIC_1) {}
	~Generic1Message() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_1);
		return size == 3 && type == MessageType::GENERIC_1;
	}
};

struct Generic2Message : public Message {
	Generic2Message() : Message(3, MessageType::GENERIC_2) {}
	~Generic2Message() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_2);
		return size == 3 && type == MessageType::GENERIC_2;
	}
};

struct Generic3Message : public Message {
	Generic3Message() : Message(3, MessageType::GENERIC_3) {}
	~Generic3Message() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_3);
		return size == 3 && type == MessageType::GENERIC_3;
	}
};

struct Generic4Message : public Message {
	Generic4Message() : Message(3, MessageType::GENERIC_4) {}
	~Generic4Message() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_4);
		return size == 3 && type == MessageType::GENERIC_4;
	}
};

struct Generic5Message : public Message {
	Generic5Message() : Message(3, MessageType::GENERIC_5) {}
	~Generic5Message() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_5);
		return size == 3 && type == MessageType::GENERIC_5;
	}
};

struct MenuMessage : public Message {
	MenuMessage() : Message(3, MessageType::MENU) {}
	~MenuMessage() override = default;
	
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::MENU);
		return size == 3 && type == MessageType::MENU;
	}
};

template<MessageType TYPE>
struct SingleStringMessage : public Message {
	protected:
	std::string string;
	
	public:
	SingleStringMessage() : Message(3, TYPE) {}
	~SingleStringMessage() override = default;
	
	bool assertValid() {
		assert(size == 3+string.length());
		assert(type == TYPE);
		return size == 3+string.length() && type == TYPE;
	}
	
	bool peek(const DynamicBuffer & buffer) override {
		if (buffer.length() < 3)
			return false;
		const auto buf = buffer.getNextBuffer();
		peekHeader(buf);
		if (buffer.length() < size)
			return false;
		std::string ret(size-3, ' ');
		for (size_t i = 3, retIndex = 0; i < size; i++, retIndex++) {
			ret[retIndex] = buffer[i];
		}
		string = ret;
		return true;
	}
	
	bool get(DynamicBuffer & buffer) override {
		bool success = peek(buffer);
		if (success)
			buffer.advanceBuffer(size);
		return success;
	}
	
	std::shared_ptr<Buffer> encode() override {
		std::vector<BufferByte> encoded;
		auto length = 3 + string.length();
		encoded.reserve(length);
		size = length;
		*reinterpret_cast<uint16_t*>(&encoded[0]) = htons(size);
		encoded[2] = static_cast<uint8_t>(type);
		strncpy(reinterpret_cast<char *>(&encoded[3]), string.c_str(), string.length());
		return std::make_shared<Buffer>(encoded.data(), length);
	}
};

template<MessageType TYPE>
struct SingleBooleanMessage : public Message {
	protected:
	bool value;
	
	public:
	SingleBooleanMessage() : Message(4, TYPE) {}
	~SingleBooleanMessage() override = default;
	
	bool assertValid() {
		assert(size == 4);
		assert(type == TYPE);
		return size == 4 && type == TYPE;
	}
	
	bool peek(const DynamicBuffer & buffer) override {
		if (buffer.length() < 4)
			return false;
		const auto buf = buffer.getNextBuffer();
		peekHeader(buf);
		value = buffer[3];
		return true;
	}
	
	bool get(DynamicBuffer & buffer) override {
		bool success = peek(buffer);
		if (success)
			buffer.advanceBuffer(size);
		return success;
	}
	
	std::shared_ptr<Buffer> encode() override {
		std::array<BufferByte, 4> encoded{};
		*reinterpret_cast<uint16_t*>(&encoded[0]) = htons(size);
		encoded[2] = static_cast<uint8_t>(type);
		encoded[3] = (value ? 1 : 0);
		return std::make_shared<Buffer>(encoded.data(), encoded.size());
	}
};

struct DisplayMessage : public SingleStringMessage<MessageType::DISPLAY_MESSAGE> {
	std::string & message = SingleStringMessage::string;
	
	DisplayMessage() : SingleStringMessage() {}
	explicit DisplayMessage(std::string message) : SingleStringMessage() { SingleStringMessage::string = std::move(message); }
	~DisplayMessage() override = default;
};

struct LoginSetUsername : public SingleStringMessage<MessageType::LOGIN_SET_USERNAME> {
	std::string & username = SingleStringMessage::string;
	
	LoginSetUsername() : SingleStringMessage() {}
	explicit LoginSetUsername(std::string username) : SingleStringMessage() { SingleStringMessage::string = std::move(username); }
	~LoginSetUsername() override = default;
};

struct LoginSetPassword : public SingleStringMessage<MessageType::LOGIN_SET_PASSWORD> {
	std::string & password = SingleStringMessage::string;
	
	LoginSetPassword() : SingleStringMessage() {}
	explicit LoginSetPassword(std::string password) : SingleStringMessage() { SingleStringMessage::string = std::move(password); }
	~LoginSetPassword() override = default;
};

struct LoginAuthenticate : public SingleStringMessage<MessageType::LOGIN_AUTHENTICATE> {
	std::string & password = SingleStringMessage::string;
	
	LoginAuthenticate() : SingleStringMessage() {}
	explicit LoginAuthenticate(std::string password) : SingleStringMessage() { SingleStringMessage::string = std::move(password); }
	~LoginAuthenticate() override = default;
};

struct LoginSetUsernameResponse : public SingleBooleanMessage<MessageType::LOGIN_SET_USERNAME_RESPONSE> {
	bool & success = SingleBooleanMessage::value;
	
	LoginSetUsernameResponse() : SingleBooleanMessage() {}
	explicit LoginSetUsernameResponse(bool success) : SingleBooleanMessage() { SingleBooleanMessage::value = success; }
	~LoginSetUsernameResponse() override = default;
};

struct LoginSetPasswordResponse : public SingleBooleanMessage<MessageType::LOGIN_SET_PASSWORD_RESPONSE> {
	bool & success = SingleBooleanMessage::value;
	
	LoginSetPasswordResponse() : SingleBooleanMessage() {}
	explicit LoginSetPasswordResponse(bool success) : SingleBooleanMessage() { SingleBooleanMessage::value = success; }
	~LoginSetPasswordResponse() override = default;
};

struct LoginAuthenticateResponse : public SingleBooleanMessage<MessageType::LOGIN_AUTHENTICATE_RESPONSE> {
	bool & success = SingleBooleanMessage::value;
	
	LoginAuthenticateResponse() : SingleBooleanMessage() {}
	explicit LoginAuthenticateResponse(bool success) : SingleBooleanMessage() { SingleBooleanMessage::value = success; }
	~LoginAuthenticateResponse() override = default;
};
