#pragma once
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <stdexcept>
#include <vector>
namespace dojo {
struct Packet { std::uint8_t type=0; std::vector<std::uint8_t> payload; bool operator==(const Packet&) const = default; };
std::vector<std::uint8_t> encode_packet(const Packet& packet);    // TODO
std::optional<Packet> decode_packet(std::span<const std::uint8_t> bytes); // TODO
inline std::vector<std::uint8_t> encode_packet(const Packet& packet) {
    if (packet.payload.size() > 1024) {
        throw std::invalid_argument("payload too large");
    }
    const auto length = static_cast<std::uint16_t>(packet.payload.size());
    std::vector<std::uint8_t> bytes(7 + static_cast<std::size_t>(length));
    bytes[0] = 0x52;
    bytes[1] = 0x32;
    bytes[2] = 1;
    bytes[3] = packet.type;
    bytes[4] = static_cast<std::uint8_t>(length >> 8);
    bytes[5] = static_cast<std::uint8_t>(length & 0xFF);
    for (std::size_t i = 0; i < packet.payload.size(); ++i) {
        bytes[6 + i] = packet.payload[i];
    }
    const std::size_t checksum_index = 6 + static_cast<std::size_t>(length);
    std::uint8_t checksum = 0;
    for (std::size_t i = 2; i < checksum_index; ++i) {
        checksum ^= bytes[i];
    }
    bytes[checksum_index] = checksum;
    return bytes;
}

inline std::optional<Packet> decode_packet(std::span<const std::uint8_t> bytes) {
    if (bytes.size() < 7) return std::nullopt;
    if (bytes[0] != 0x52 || bytes[1] != 0x32) return std::nullopt;
    if (bytes[2] != 1) return std::nullopt;

    const std::uint16_t length =
        static_cast<std::uint16_t>((bytes[4] << 8) | bytes[5]);
    if (length > 1024) return std::nullopt;
    const std::size_t checksum_index = 6 + static_cast<std::size_t>(length);
    if (bytes.size() != checksum_index + 1) return std::nullopt;

    std::uint8_t checksum = 0;
    for (std::size_t i = 2; i < checksum_index; ++i) {
        checksum ^= bytes[i];
    }
    if (checksum != bytes[checksum_index]) return std::nullopt;

    Packet packet;
    packet.type = bytes[3];
    const auto payload = bytes.subspan(6, length);
    packet.payload.assign(payload.begin(), payload.end());
    return packet;
}

}
