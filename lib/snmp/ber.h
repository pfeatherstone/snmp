#pragma once

#include <limits>
#include <bit>
#include <string>
#include <string_view>
#include <tuple>
#include "traits.h"
#include "error.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize  (std::nullptr_t, sink_type auto&& sink);
    void ber_deserialize(std::nullptr_t, source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize  (bool v,  sink_type auto&& sink);
    void ber_deserialize(bool& v, source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize  (std::integral auto v,  sink_type auto&& sink);
    void ber_deserialize(std::integral auto& v, source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize(std::string_view v, sink_type auto&& sink);
    void ber_serialize(const char* v,      sink_type auto&& sink);
    void ber_deserialize(std::string& v,   source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

    void ber_serialize(const const_sequence_container auto& v, sink_type auto&& sink);
    void ber_deserialize(appendable_sequence_container auto& v, source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

    template<class... Types>
    void ber_serialize(const std::tuple<Types...>& v, sink_type auto&& sink);

    template<class... Types>
    void ber_deserialize(std::tuple<Types...>& v, source_type auto&& src);

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

        inline uint8_t read_one(source_type auto& src)
        {
            uint8_t tag{};
            src((char*)&tag, 1);
            return tag;
        }

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
                    buf[i+1] = static_cast<char>((len >> ((nbytes - 1 - i) * 8)) & 0xffu); // big endian order
                sink(buf, 1 + nbytes);
            }
        }

        inline size_t ber_deserialize_length(source_type auto& src)
        {
            const uint8_t b0 = read_one(src);

            if (b0 < 0x80u)
                return b0;
            
            // Indefinite length is unsupported; 0xFF is reserved.
            if (b0 == 0x80u || b0 == 0xFFu)
                throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
            
            const uint8_t nbytes = b0 & 0x7fu;

            if (nbytes > 8)
                throw std::system_error(snmp_error::LENGTH_HEADER_OVERFLOW);

            uint8_t buf[8]{};
            src((char*)buf, nbytes);

            size_t len{0};
            for (size_t i = 0 ; i < nbytes ; ++i)
                len = (len << 8) | static_cast<size_t>(buf[i]);

            return len;
        }

//----------------------------------------------------------------------------------------------------------------

        inline auto counter_sink(std::size_t& counter)
        {
            return [&counter](const char*, size_t len) {
                counter += len;
            };
        }

//----------------------------------------------------------------------------------------------------------------

        inline auto bounded_source(const size_t bounded_len, size_t& off, source_type auto& src)
        {
            return [bounded_len, &off, &src](char* buf, size_t len) {
                if (len > (bounded_len-off))
                    throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
                src(buf, len);
                off += len;
            };
        };

//----------------------------------------------------------------------------------------------------------------

    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(std::nullptr_t, sink_type auto&& sink)
    {
        using namespace details;
        const char buf[2] = {TAG_NULL, 0};
        sink(buf, 2);
    }

    inline void ber_deserialize(std::nullptr_t, source_type auto&& src)
    {
        using namespace details;
        if (read_one(src) != TAG_NULL)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);
        if (ber_deserialize_length(src) != 0)
            throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
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

    inline void ber_deserialize(bool& v, source_type auto&& src)
    {
        using namespace details;
        if (read_one(src) != TAG_BOOLEAN)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);
        if (ber_deserialize_length(src) != 1)
            throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
        v = read_one(src) > 0;
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
        char buf[sizeof(U) + 1]{};

        for (size_t i = 0; i < nbytes; ++i)
        {
            const size_t idx = nbytes - 1 - i;
            if (idx < sizeof(U))
                buf[i] = static_cast<char>((bits >> (idx * 8)) & 0xffu);
        }

        sink(buf, nbytes);
    }

    inline void ber_deserialize(std::integral auto& v, source_type auto&& src)
    {
        using namespace details;
        using T = std::remove_cvref_t<decltype(v)>;

        if (read_one(src) != TAG_INTEGER)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);

        const size_t len = ber_deserialize_length(src);

        if (len == 0)
            throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
        if (len > (sizeof(T) + 1))
            throw std::system_error(snmp_error::INTEGER_OVERFLOW);

        uint8_t buf[sizeof(T) + 1];
        src((char*)buf, len);
        const bool negative = (buf[0] & 0x80u) != 0;

        if constexpr (std::is_unsigned_v<T>)
            if (negative)
                throw std::system_error(snmp_error::INTEGER_OVERFLOW);

        uint64_t bits = negative ? std::numeric_limits<uint64_t>::max() : uint64_t{0};
        for (size_t i = 0 ; i < len ; ++i)
            bits = (bits << 8) | static_cast<uint64_t>(buf[i]);

        v = static_cast<T>(bits);
    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(std::string_view v, sink_type auto&& sink)
    {
        using namespace details;
        sink((const char*)&TAG_STRING, 1);
        ber_serialize_length(v.size(), sink);
        if (!v.empty())
            sink(v.data(), v.size());
    }

    inline void ber_serialize(const char* v, sink_type auto&& sink)
    {
        ber_serialize(std::string_view(v), sink);
    }

    inline void ber_deserialize(std::string& v, source_type auto&& src)
    {
        using namespace details;

        if (read_one(src) != TAG_STRING)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);

        const size_t len = ber_deserialize_length(src);
        v.resize(len);
        if (len > 0)
            src(v.data(), len);
    }
    
//----------------------------------------------------------------------------------------------------------------

    inline void ber_serialize(const const_sequence_container auto& v, sink_type auto&& sink)
    {
        using namespace details;
        using snmp::ber_serialize;

        // This function has to serialize TWICE!!!

        // Count total number of bytes required to serialize v
        size_t counter{};
        for (const auto& el : v)
            ber_serialize(el, counter_sink(counter));

        // Serialize to sink
        sink((const char*)&TAG_SEQUENCE, 1);
        ber_serialize_length(counter, sink);
        for (const auto& el : v)
            ber_serialize(el, sink);
    }

//----------------------------------------------------------------------------------------------------------------

    inline void ber_deserialize(appendable_sequence_container auto& v, source_type auto&& src)
    {
        using namespace details;
        using snmp::ber_deserialize;
        using T = sequence_value_t<decltype(v)>;

        if (read_one(src) != TAG_SEQUENCE)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);

        const size_t len = ber_deserialize_length(src);
        size_t off{};
        v.clear();

        for (; off < len;)
        {
            T el{};
            ber_deserialize(el, bounded_source(len, off, src));
            v.push_back(std::move(el));
        }
    }

//----------------------------------------------------------------------------------------------------------------

    namespace details
    {
        template <class Tuple, class F>
        constexpr void for_each_in_tuple(Tuple&& t, F&& f)
        {
            [&]<std::size_t... I>(std::index_sequence<I...>) {
                (f(std::get<I>(std::forward<Tuple>(t))), ...);
            }(std::make_index_sequence<std::tuple_size_v<std::remove_cvref_t<Tuple>>>{});
        }
    }

    template<class ... Types>
    inline void ber_serialize(const std::tuple<Types...>& tpl, sink_type auto&& sink)
    {
        using namespace details;
        using snmp::ber_serialize;

        // This function has to serialize TWICE!!!

        // Count total number of bytes required to serialize v
        size_t counter{};
        for_each_in_tuple(tpl, [&](const auto& el) {
            ber_serialize(el, counter_sink(counter));
        });

        // Serialize to sink
        sink((const char*)&TAG_SEQUENCE, 1);
        ber_serialize_length(counter, sink);
        for_each_in_tuple(tpl, [&](const auto& el) {
            ber_serialize(el, sink);
        });
    }

    template<class... Types>
    inline void ber_deserialize(std::tuple<Types...>& tpl, source_type auto&& src)
    {
        using namespace details;
        using snmp::ber_deserialize;

        if (read_one(src) != TAG_SEQUENCE)
            throw std::system_error(snmp_error::UNEXPECTED_TAG);

        const size_t len = ber_deserialize_length(src);
        size_t off{};

        for_each_in_tuple(tpl, [&](auto& el) {
            ber_deserialize(el, bounded_source(len, off, src));
        });

        if (off != len)
            throw std::system_error(snmp_error::UNEXPECTED_LENGTH);
    }

//----------------------------------------------------------------------------------------------------------------

}