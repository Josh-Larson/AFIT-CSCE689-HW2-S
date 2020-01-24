#pragma once

#include <Selector.h>

#include <cstdint>
#include <cassert>
#include <memory>

enum class MessageType : uint8_t {
	UNKNOWN     = 0,
	HELLO       = 1,
	GENERIC_1   = 2,
	GENERIC_2   = 3,
	GENERIC_3   = 4,
	GENERIC_4   = 5,
	GENERIC_5   = 6,
	MENU        = 7
};

struct Message {
	uint16_t    size = 0;
	MessageType type = MessageType::UNKNOWN;
	
	Message() = default;
	Message(uint16_t size, MessageType type) : size(size), type(type) {}
	
	virtual bool peek(const DynamicBuffer & buffer) {
		if (buffer.length() < 3)
			return false;
		const auto buf = buffer.getNextBuffer();
		size = *reinterpret_cast<const uint16_t*>(buf->data());
		type = static_cast<MessageType>(buf->get(2));
		return true;
	}
	
	virtual bool get(DynamicBuffer & buffer) {
		bool success = peek(buffer);
		if (success)
			buffer.advanceBuffer(3);
		return success;
	}
	
	virtual std::shared_ptr<Buffer> encode() {
		std::array<BufferByte, 3> encoded {};
		*reinterpret_cast<uint16_t*>(encoded.data()) = size;
		encoded[2] = static_cast<uint8_t>(type);
		return std::make_shared<Buffer>(encoded.data(), 3);
	}
};

struct HelloMessage : public Message {
	HelloMessage() : Message(sizeof(HelloMessage), MessageType::HELLO) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::HELLO);
		return size == 3 && type == MessageType::HELLO;
	}
};

struct Generic1Message : public Message {
	Generic1Message() : Message(sizeof(Generic1Message), MessageType::GENERIC_1) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_1);
		return size == 3 && type == MessageType::GENERIC_1;
	}
};

struct Generic2Message : public Message {
	Generic2Message() : Message(sizeof(Generic2Message), MessageType::GENERIC_2) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_2);
		return size == 3 && type == MessageType::GENERIC_2;
	}
};

struct Generic3Message : public Message {
	Generic3Message() : Message(sizeof(Generic3Message), MessageType::GENERIC_3) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_3);
		return size == 3 && type == MessageType::GENERIC_3;
	}
};

struct Generic4Message : public Message {
	Generic4Message() : Message(sizeof(Generic4Message), MessageType::GENERIC_4) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_4);
		return size == 3 && type == MessageType::GENERIC_4;
	}
};

struct Generic5Message : public Message {
	Generic5Message() : Message(sizeof(Generic5Message), MessageType::GENERIC_5) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::GENERIC_5);
		return size == 3 && type == MessageType::GENERIC_5;
	}
};

struct MenuMessage : public Message {
	MenuMessage() : Message(sizeof(MenuMessage), MessageType::MENU) {}
	bool assertValid() {
		assert(size == 3);
		assert(type == MessageType::MENU);
		return size == 3 && type == MessageType::MENU;
	}
};
