#pragma once

#include <string>
#include <array>
#include <vector>

class Security {
	static Security instance;
	static std::array<char, 64> base64EncodeAlphabet;
	static std::array<uint8_t, 256> base64DecodeAlphabet;
	
	public:
	static inline Security * INSTANCE() noexcept { return &instance; }
	
	[[nodiscard]] std::string hash(const std::string& password, const std::string& salt) const noexcept;
	[[nodiscard]] std::string generateSalt() const noexcept;
	[[nodiscard]] std::string base64Encode(const void * data, size_t length) const noexcept;
	[[nodiscard]] std::vector<uint8_t> base64Decode(const std::string & base64) const noexcept;
	
	void setFDEcho(int fd, bool echo);
	
	private:
	static constexpr std::array<uint8_t, 256> generateBase64DecodeAlphabet() noexcept;
};