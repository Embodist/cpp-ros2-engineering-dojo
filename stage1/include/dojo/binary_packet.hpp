#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
namespace dojo {
struct Packet { std::uint8_t type=0; std::vector<std::uint8_t> payload; bool operator==(const Packet&) const = default; };
std::vector<std::uint8_t> encode_packet(const Packet& packet);    // TODO
std::optional<Packet> decode_packet(std::span<const std::uint8_t> bytes); // TODO
// ===== STUDENT WORK AREA: remove and replace these intentionally incomplete stubs =====
inline std::vector<std::uint8_t> encode_packet(const Packet& packet) { (void)packet; return {}; /* TODO */ }
inline std::optional<Packet> decode_packet(std::span<const std::uint8_t> bytes) { (void)bytes; return std::nullopt; /* TODO */ }

}
