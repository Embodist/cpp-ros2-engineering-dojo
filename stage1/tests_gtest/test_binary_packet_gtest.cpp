#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

#include "dojo/binary_packet.hpp"

TEST(BinaryPacket, Roundtrip) {
    dojo::Packet packet{4, {0, 1, 0xff, 2}};
    auto bytes = dojo::encode_packet(packet);
    auto decoded = dojo::decode_packet(bytes);
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value(), packet);
}

TEST(BinaryPacket, EmptyPayloadLayout) {
    auto bytes = dojo::encode_packet(dojo::Packet{7, {}});
    ASSERT_EQ(bytes.size(), 7u);
    EXPECT_EQ(bytes[0], 0x52u);
    EXPECT_EQ(bytes[1], 0x32u);
    EXPECT_EQ(bytes[4], 0u);
    EXPECT_EQ(bytes[5], 0u);
    EXPECT_TRUE(dojo::decode_packet(bytes).has_value());
}

TEST(BinaryPacket, RejectsChecksumCorruption) {
    auto bytes = dojo::encode_packet(dojo::Packet{8, {1, 2}});
    bytes.back() ^= 1;
    EXPECT_FALSE(dojo::decode_packet(bytes).has_value());
}

TEST(BinaryPacket, RejectsTrailingBytes) {
    auto bytes = dojo::encode_packet(dojo::Packet{8, {1, 2}});
    bytes.push_back(0);
    EXPECT_FALSE(dojo::decode_packet(bytes).has_value());
}

TEST(BinaryPacket, RejectsTruncatedInput) {
    std::vector<std::uint8_t> bytes{0x52};
    EXPECT_FALSE(dojo::decode_packet(bytes).has_value());
}

TEST(BinaryPacket, RejectsWrongVersionAndMagic) {
    auto bytes = dojo::encode_packet(dojo::Packet{1, {1}});
    bytes[2] = 2;
    EXPECT_FALSE(dojo::decode_packet(bytes).has_value());

    bytes = dojo::encode_packet(dojo::Packet{1, {1}});
    bytes[0] = 0;
    EXPECT_FALSE(dojo::decode_packet(bytes).has_value());
}

TEST(BinaryPacket, RejectsOversizePayload) {
    EXPECT_THROW(
        dojo::encode_packet(dojo::Packet{1, std::vector<std::uint8_t>(1025)}),
        std::invalid_argument);
}

TEST(BinaryPacket, MaxPayloadRoundtrip) {
    dojo::Packet packet{1, std::vector<std::uint8_t>(1024, 0xAB)};
    auto decoded = dojo::decode_packet(dojo::encode_packet(packet));
    ASSERT_TRUE(decoded.has_value());
    EXPECT_EQ(decoded.value(), packet);
}
