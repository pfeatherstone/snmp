#pragma once

#include <bit>
#include <string_view>
#include "traits.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize(std::nullptr_t,              sink_type auto&& sink);
    void ber_serialize(bool v,                      sink_type auto&& sink);
    void ber_serialize(std::integral auto v,        sink_type auto&& sink);
    void ber_serialize(std::string_view v,          sink_type auto&& sink);
    void ber_serialize(const char* v,               sink_type auto&& sink);

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

    namespace details
    {

//----------------------------------------------------------------------------------------------------------------

        constexpr uint16_t byte_swap16(uint16_t v)
        {
            return static_cast<uint16_t>(((v & 0x00FF) << 8) | ((v & 0xFF00) >> 8));
        }

        constexpr uint32_t byte_swap32(uint32_t v)
        {
            return static_cast<uint32_t>(((v & 0x000000FFu) << 24) |
                                        ((v & 0x0000FF00u) << 8)  |
                                        ((v & 0x00FF0000u) >> 8)  |
                                        ((v & 0xFF000000u) >> 24));
        }

        constexpr uint64_t byte_swap64(uint64_t v)
        {
            return static_cast<uint64_t>(((v & 0x00000000000000FFULL) << 56) |
                                        ((v & 0x000000000000FF00ULL) << 40) |
                                        ((v & 0x0000000000FF0000ULL) << 24) |
                                        ((v & 0x00000000FF000000ULL) << 8)  |
                                        ((v & 0x000000FF00000000ULL) >> 8)  |
                                        ((v & 0x0000FF0000000000ULL) >> 24) |
                                        ((v & 0x00FF000000000000ULL) >> 40) |
                                        ((v & 0xFF00000000000000ULL) >> 56));
        }

        static_assert(byte_swap16(0x1234)               == 0x3412,              "bad swap");
        static_assert(byte_swap32(0x12345678)           == 0x78563412,          "bad swap");
        static_assert(byte_swap64(0x123456789abcdef1)   == 0xf1debc9a78563412,  "bad swap");

        inline bool is_little_endian() 
        {
            constexpr uint32_t v{0x01020304};
            const auto*        ptr{reinterpret_cast<const unsigned char*>(&v)};
            return ptr[0] == 0x04;
        }

//----------------------------------------------------------------------------------------------------------------

        inline uint16_t host_to_b16(uint16_t v) { return is_little_endian() ? byte_swap16(v) : v; }
        inline uint32_t host_to_b32(uint32_t v) { return is_little_endian() ? byte_swap32(v) : v; }
        inline uint64_t host_to_b64(uint64_t v) { return is_little_endian() ? byte_swap64(v) : v; }

//----------------------------------------------------------------------------------------------------------------

        constexpr uint8_t TAG_BOOLEAN      = 0b00000001;
        constexpr uint8_t TAG_INTEGER      = 0b00000010;
        constexpr uint8_t TAG_STRING       = 0b00000100;
        constexpr uint8_t TAG_NULL         = 0b00000101;
        constexpr uint8_t TAG_ENUMERATED   = 0b00001010;
        constexpr uint8_t TAG_SEQUENCE     = 0b00110000;
        constexpr uint8_t TAG_SET          = 0b00110001;

        constexpr uint8_t CLASS_UNIVERSAL   = 0b00000000;
        constexpr uint8_t CLASS_APPLICATION = 0b01000000;
        constexpr uint8_t CLASS_CONTEXT     = 0b10000000;
        constexpr uint8_t CLASS_PRIVATE     = 0b11000000;

        constexpr uint8_t CONTENT_PRIMITIVE = 0b00000000;
        constexpr uint8_t CONTENT_NESTED    = 0b00100000;

        constexpr uint8_t TYPE_MULTI_MASK   = 0b00011111;

//----------------------------------------------------------------------------------------------------------------

        inline void ber_serialize_length(size_t len, sink_type auto& sink)
        {
            if (len < 128u)
            {
                const char tmp = len;
                sink(&tmp, 1);
            }
            else
            {
                const size_t nbytes = (std::bit_width(len) + 7) / 8;
                char buf[9];
                buf[0] = static_cast<char>(0x80u | nbytes);
                for (size_t i = 0; i < nbytes; ++i)
                    buf[i+1] = static_cast<char>((len >> ((nbytes - 1 - i) * 8)) & 0xffu);
                sink(buf, 1 + nbytes);
            }
        }

//----------------------------------------------------------------------------------------------------------------

    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(std::nullptr_t, sink_type auto&& sink)
    {
        using namespace details;
        const char buf[2] = {TAG_NULL, 0};
        sink(buf, 2);
    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(bool v, sink_type auto&& sink)
    {
        using namespace details;
        constexpr uint8_t true_  = 0xffu;
        constexpr uint8_t false_ = 0u;
        const uint8_t buf[3] = {TAG_BOOLEAN, 1, v ? true_ : false_};
        sink((const char*)buf, 3);
    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(std::integral auto v, sink_type auto&& sink)
    {
        using namespace details;
        using T = decltype(v);
        using U = std::make_unsigned_t<T>;

        // Calculate number of bytes
        const U bits = static_cast<U>(v);
        U possibly_inverted_bits{bits};

        if constexpr (std::is_signed_v<T>)
            if (v < 0)
                possibly_inverted_bits = static_cast<U>(~bits);

        const size_t nbytes = (std::bit_width(possibly_inverted_bits) + 8) / 8; // +1 for sign bit

        // Serialize tag and length
        sink((const char*)&TAG_INTEGER, 1);
        ber_serialize_length(nbytes, sink);

        // Serialize integer - special consideration for possible leading zero because of sign bit
        char buf[sizeof(U) + 1];

        for (size_t i = 0; i < nbytes; ++i)
        {
            const size_t idx = nbytes - 1 - i;

            if (idx >= sizeof(U))
                buf[i] = 0; // Extra leading zero for a positive unsigned value
            else
                buf[i] = static_cast<char>((bits >> (idx * 8)) & 0xffu);
        }

        sink(buf, nbytes);
    }

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize(std::string_view v, sink_type auto&& sink)
    {
        using namespace details;
        sink((const char*)&TAG_STRING, 1);
        ber_serialize_length(v.size(), sink);
        if (!v.empty())
            sink(v.data(), v.size());
    }

    void ber_serialize(const char* v, sink_type auto&& sink)
    {
        ber_serialize(std::string_view(v), sink);
    }
    
//----------------------------------------------------------------------------------------------------------------

}