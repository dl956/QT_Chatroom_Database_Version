#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <boost/asio.hpp>

// Helpers to encode/decode 4-byte big-endian length prefix
inline std::vector<uint8_t> make_frame(const std::string& payload) {
    uint32_t payload_length = static_cast<uint32_t>(payload.size());
    std::vector<uint8_t> frame(4 + payload.size());
    frame[0] = static_cast<uint8_t>((payload_length >> 24) & 0xFF);
    frame[1] = static_cast<uint8_t>((payload_length >> 16) & 0xFF);
    frame[2] = static_cast<uint8_t>((payload_length >> 8) & 0xFF);
    frame[3] = static_cast<uint8_t>((payload_length) & 0xFF);
    std::copy(payload.begin(), payload.end(), frame.begin() + 4);
    return frame;
}

inline uint32_t parse_length(const std::vector<uint8_t>& buffer) {
    if (buffer.size() < 4) return 0;
    return (static_cast<uint32_t>(buffer[0]) << 24) |
           (static_cast<uint32_t>(buffer[1]) << 16) |
           (static_cast<uint32_t>(buffer[2]) << 8) |
           (static_cast<uint32_t>(buffer[3]));
}