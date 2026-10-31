#pragma once

#include <cstdint>
#include <concepts>
#include <type_traits>
#include <string_view>

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    template<class Byte>
    constexpr bool is_byte = std::is_same_v<Byte, char>     || 
                             std::is_same_v<Byte, uint8_t>  ||
                             std::is_same_v<Byte, int8_t>;

    template<class Byte>
    concept byte_type = is_byte<Byte>;

//----------------------------------------------------------------------------------------------------------------

    template<class F> concept sink_type   = std::invocable<F, const char*, std::size_t>;
    template<class F> concept source_type = std::invocable<F, char*, std::size_t>;

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize(std::integral auto v,        sink_type auto& sink);
    void ber_serialize(std::floating_point auto v,  sink_type auto& sink);
    void ber_serialize(std::string_view v,          sink_type auto& sink);

//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------------------------------

    namespace details
    {
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

#if __cpp_lib_endian
        constexpr bool is_little_endian()
        {
            return std::endian::native == std::endian::little;
        }
#else
        inline bool is_little_endian() 
        {
            constexpr uint32_t v{0x01020304};
            const auto*        ptr{reinterpret_cast<const unsigned char*>(&v)};
            return ptr[0] == 0x04;
        }
#endif

//----------------------------------------------------------------------------------------------------------------

        inline uint16_t host_to_b16(uint16_t v) { return is_little_endian() ? byte_swap16(v) : v; }
        inline uint32_t host_to_b32(uint32_t v) { return is_little_endian() ? byte_swap32(v) : v; }
        inline uint64_t host_to_b64(uint64_t v) { return is_little_endian() ? byte_swap64(v) : v; }

//----------------------------------------------------------------------------------------------------------------

        constexpr uint8_t TYPE_BOOLEAN      = 0b00000001;
        constexpr uint8_t TYPE_INTEGER      = 0b00000010;
        constexpr uint8_t TYPE_STRING       = 0b00000100;
        constexpr uint8_t TYPE_NULL         = 0b00000101;
        constexpr uint8_t TYPE_ENUMERATED   = 0b00001010;
        constexpr uint8_t TYPE_SEQUENCE     = 0b00110000;
        constexpr uint8_t TYPE_SET          = 0b00110001;

        constexpr uint8_t CLASS_UNIVERSAL   = 0b00000000;
        constexpr uint8_t CLASS_APPLICATON  = 0b01000000;
        constexpr uint8_t CLASS_CONTEXT     = 0b10000000;
        constexpr uint8_t CLASS_PRIVATE     = 0b11000000;

        constexpr uint8_t CONTENT_PRIMITIVE = 0b00000000;
        constexpr uint8_t CONTENT_NESTED    = 0b00100000;

        constexpr uint8_t TYPE_MULTI_MASK   = 0b00011111;


    }
}