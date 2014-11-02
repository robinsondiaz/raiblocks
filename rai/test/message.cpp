#include <gtest/gtest.h>
#include <rai/core/core.hpp>

TEST (message, keepalive_serialization)
{
    rai::keepalive request1;
    std::vector <uint8_t> bytes;
    {
        rai::vectorstream stream (bytes);
        request1.serialize (stream);
    }
    rai::keepalive request2;
    rai::bufferstream buffer (bytes.data (), bytes.size ());
    ASSERT_FALSE (request2.deserialize (buffer));
    ASSERT_EQ (request1, request2);
}

TEST (message, keepalive_deserialize)
{
    rai::keepalive message1;
    message1.peers [0] = rai::endpoint (boost::asio::ip::address_v6::loopback (), 10000);
    std::vector <uint8_t> bytes;
    {
        rai::vectorstream stream (bytes);
        message1.serialize (stream);
    }
    uint8_t version_max;
    uint8_t version_using;
    uint8_t version_min;
	rai::message_type type;
	std::bitset <64> extensions;
    rai::bufferstream header_stream (bytes.data (), bytes.size ());
    ASSERT_FALSE (rai::message::read_header (header_stream, version_max, version_using, version_min, type, extensions));
    ASSERT_EQ (rai::message_type::keepalive, type);
    rai::keepalive message2;
    rai::bufferstream stream (bytes.data (), bytes.size ());
    ASSERT_FALSE (message2.deserialize (stream));
    ASSERT_EQ (message1.peers, message2.peers);
}

TEST (message, publish_serialization)
{
    rai::publish publish (std::unique_ptr <rai::block> (new rai::send_block));
    ASSERT_EQ (rai::block_type::send, publish.block_type ());
    ASSERT_FALSE (publish.ipv4_only ());
    publish.ipv4_only_set (true);
    ASSERT_TRUE (publish.ipv4_only ());
    std::vector <uint8_t> bytes;
    {
        rai::vectorstream stream (bytes);
        publish.write_header (stream);
    }
    ASSERT_EQ (16, bytes.size ());
    ASSERT_EQ (0xb5, bytes [0]);
    ASSERT_EQ (0x52, bytes [1]);
    ASSERT_EQ (0x41, bytes [2]);
    ASSERT_EQ (0x73, bytes [3]);
    ASSERT_EQ (0x01, bytes [4]);
    ASSERT_EQ (0x01, bytes [5]);
    ASSERT_EQ (0x01, bytes [6]);
    ASSERT_EQ (static_cast <uint8_t> (rai::message_type::publish), bytes [7]);
    ASSERT_EQ (0x02, bytes [8]);
    ASSERT_EQ (static_cast <uint8_t> (rai::block_type::send), bytes [9]);
    for (auto i (bytes.begin () + 10), n (bytes.end ()); i != n; ++i)
    {
        ASSERT_EQ (0, *i);
    }
    rai::bufferstream stream (bytes.data (), bytes.size ());
    uint8_t version_max;
    uint8_t version_using;
    uint8_t version_min;
    rai::message_type type;
    std::bitset <64> extensions;
    ASSERT_FALSE (rai::message::read_header (stream, version_max, version_using, version_min, type, extensions));
    ASSERT_EQ (0x01, version_min);
    ASSERT_EQ (0x01, version_using);
    ASSERT_EQ (0x01, version_max);
    ASSERT_EQ (rai::message_type::publish, type);
}