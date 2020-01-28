#include <Security.h>

#include <argon2.h>
#include <cmath>
#include <random>
#include <cassert>

Security Security::instance{};
std::array<char, 64> Security::base64EncodeAlphabet = {
		'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
		'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
		'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
};
std::array<uint8_t, 256> Security::base64DecodeAlphabet = generateBase64DecodeAlphabet();

constexpr std::array<uint8_t, 256> Security::generateBase64DecodeAlphabet() noexcept {
	std::array<uint8_t, 256> ret{}; // Zero-initializes everything
	for (size_t i = 0; i < base64EncodeAlphabet.size(); i++) {
		ret[base64EncodeAlphabet[i]] = i;
	}
	return ret;
}

std::string Security::hash(const std::string& password, const std::string& salt) const noexcept {
	uint32_t t_cost = 2;            // 1-pass computation
	uint32_t m_cost = (1u<<16u);      // 64 mebibytes memory usage
	uint32_t parallelism = 1;       // number of threads and lanes
	std::array<uint8_t, 32> ret{};
	
	argon2i_hash_raw(t_cost, m_cost, parallelism, password.c_str(), password.length(), salt.c_str(), salt.length(), ret.data(), ret.size());
	
	return base64Encode(ret.data(), ret.size());
}

std::string Security::generateSalt() const noexcept {
	std::default_random_engine generator{std::random_device{}()};
	std::uniform_int_distribution<uint8_t> distribution(0,255);
	std::array<uint8_t, 16> salt{};
	for (int i = 0; i < 16; i++) {
		salt[i] = distribution(generator);
	}
	return base64Encode(salt.data(), salt.size());
}

std::string Security::base64Encode(const void *dataRaw, size_t length) const noexcept {
	auto data = static_cast<const uint8_t*>(dataRaw);
	std::string ret(static_cast<int>(std::ceil(length / 3.0)) * 4, '=');
	int index = 0;
	int retIndex = 0;
	// 3-byte chunks
	for (; index+2 < length; index += 3) {
		ret[retIndex++] = base64EncodeAlphabet[ (data[index] & 0b11111100u) >> 2u];
		ret[retIndex++] = base64EncodeAlphabet[((data[index] & 0b00000011u) << 4u) | (data[index + 1] & 0b11110000u) >> 4u];
		ret[retIndex++] = base64EncodeAlphabet[((data[index + 1] & 0b00001111u) << 2u) | (data[index + 2] & 0b11000000u) >> 6u];
		ret[retIndex++] = base64EncodeAlphabet[ (data[index + 2] & 0b00111111u)];
	}
	// One byte remaining?
	if (index < length) {
		ret[retIndex++] = base64EncodeAlphabet[(data[index++] & 0b11111100u) >> 2u];
		// Two bytes remaining?
		if (index < length) {
			ret[retIndex++] = base64EncodeAlphabet[((data[index - 1] & 0b00000011u) << 4u) | (data[index] & 0b11110000u) >> 4u];
			ret[retIndex]   = base64EncodeAlphabet[((data[index] & 0b00001111u) << 2u)];
		} else {
			ret[retIndex]   = base64EncodeAlphabet[((data[index - 1] & 0b00000011u) << 4u)];
		}
	}
	return ret;
}

std::vector<uint8_t> Security::base64Decode(const std::string &base64) const noexcept {
	std::vector<uint8_t> ret;
//	ret.resize(static_cast<int>(std::ceil(base64.length() / 4.0)) * 3);
	std::array<uint8_t, 4> decodedChunk{};
	const auto base64Length = base64.length();
	int index = 0;
	// 4-byte chunks
	for (; index+3 < base64Length; index += 4) {
		// Grab chunk
		for (int i = 0; i < 4; i++)
			decodedChunk[i] = base64DecodeAlphabet[base64[index+i]];
		ret.push_back(static_cast<uint8_t>(decodedChunk[0] << 2u) | static_cast<uint8_t>(decodedChunk[1] >> 4u));
		ret.push_back(static_cast<uint8_t>(decodedChunk[1] << 4u) | static_cast<uint8_t>(decodedChunk[2] >> 2u));
		ret.push_back(static_cast<uint8_t>(decodedChunk[2] << 6u) | static_cast<uint8_t>(decodedChunk[3]));
	}
	for (int i = 0; index+i < base64Length; i++)
		decodedChunk[i] = base64DecodeAlphabet[base64[index+i]];
	if (index < base64Length && base64[index] != '=') {
		if (index+1 < base64Length && base64[index+1] != '=') {
			if (index+2 < base64Length && base64[index+2] != '=') {
				assert(index+3 > base64Length);
				ret.push_back(static_cast<uint8_t>(decodedChunk[0] << 2u) | static_cast<uint8_t>(decodedChunk[1] >> 4u));
				ret.push_back(static_cast<uint8_t>(decodedChunk[1] << 4u) | static_cast<uint8_t>(decodedChunk[2] >> 2u));
				ret.push_back(static_cast<uint8_t>(decodedChunk[2] << 6u));
			} else {
				ret.push_back(static_cast<uint8_t>(decodedChunk[0] << 2u) | static_cast<uint8_t>(decodedChunk[1] >> 4u));
				ret.push_back(static_cast<uint8_t>(decodedChunk[1] << 4u));
			}
		} else {
			ret.push_back(static_cast<uint8_t>(decodedChunk[0] << 2u));
		}
	}
	
	return ret;
}
