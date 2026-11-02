#pragma once

#include <limits>
#include <bit>
#include "error.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------
    
    namespace details
    {
        
//----------------------------------------------------------------------------------------------------------------

        constexpr uint8_t TAG_BOOLEAN      = 0b00000001;
        constexpr uint8_t TAG_INTEGER      = 0b00000010;
        constexpr uint8_t TAG_STRING       = 0b00000100;
        constexpr uint8_t TAG_NULL         = 0b00000101;
        constexpr uint8_t TAG_ENUMERATED   = 0b00001010;
        constexpr uint8_t TAG_SEQUENCE     = 0b00110000;
        constexpr uint8_t TAG_SET          = 0b00110001;

//----------------------------------------------------------------------------------------------------------------

        template<class T>
        struct type_ {using type = T;};

        template<class...>
        constexpr bool always_false = false;

        template<class T>
        constexpr uint8_t ber_tag(type_<T>)
        {
            using U = std::remove_cvref_t<T>;

            if constexpr (std::is_same_v<U, std::nullptr_t>)
                return TAG_NULL;
            else if constexpr (std::is_same_v<U, bool>)
                return TAG_BOOLEAN;
            else if constexpr(std::is_integral_v<U>)
                return TAG_INTEGER;
            else if constexpr(std::convertible_to<U, std::string_view>)
                return TAG_STRING;
            else if constexpr(const_sequence_container<U>)
                return TAG_SEQUENCE;
            else
                static_assert(always_false<U>, "unrecognized tag");
        }

        template<class... Types>
        constexpr uint8_t ber_tag(type_<std::tuple<Types...>>)
        {
            return TAG_SEQUENCE;
        }

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

    template<class... Types>
    inline void ber_serialize(const std::variant<Types...>& v, sink_type auto&& sink)
    {
        using namespace details;
        using snmp::ber_serialize;

        std::visit([&](const auto& el) {
            ber_serialize(el, sink);
        }, v);
    }

    template<class... Types>
    inline void ber_deserialize(std::variant<Types...>& v, source_type auto&& src)
    {
        using namespace details;
    }

//----------------------------------------------------------------------------------------------------------------

}