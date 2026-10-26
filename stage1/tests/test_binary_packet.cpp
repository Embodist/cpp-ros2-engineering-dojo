#include "dojo/binary_packet.hpp"
#include "test_harness.hpp"
#include <vector>
DOJO_TEST("roundtrip") { dojo::Packet p{4,{0,1,0xff,2}}; auto bytes=dojo::encode_packet(p); auto out=dojo::decode_packet(bytes); REQUIRE(out.has_value()); REQUIRE(out.value()==p); }
DOJO_TEST("empty frame length") { auto b=dojo::encode_packet(dojo::Packet{7,{}}); REQUIRE_EQ(b.size(),7u); REQUIRE_EQ(b[0],0x52u); REQUIRE_EQ(b[1],0x32u); REQUIRE_EQ(b[4],0u); REQUIRE_EQ(b[5],0u); }
DOJO_TEST("malformed and trailing") { auto b=dojo::encode_packet(dojo::Packet{8,{1,2}}); b.back()^=1; REQUIRE(!dojo::decode_packet(b)); b=dojo::encode_packet(dojo::Packet{8,{1,2}}); b.push_back(0); REQUIRE(!dojo::decode_packet(b)); REQUIRE(!dojo::decode_packet(std::vector<std::uint8_t>{0x52})); }
DOJO_TEST("oversize rejects") { REQUIRE_THROWS(dojo::encode_packet(dojo::Packet{1,std::vector<std::uint8_t>(1025)})); }
DOJO_TEST("wrong version and wrong header") {
    auto b=dojo::encode_packet(dojo::Packet{1,{1}});
    b[2]=2; REQUIRE(!dojo::decode_packet(b));
    b=dojo::encode_packet(dojo::Packet{1,{1}});
    b[0]=0; REQUIRE(!dojo::decode_packet(b));
}
DOJO_MAIN()
