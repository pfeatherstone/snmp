#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <concepts>

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

    template<class F> concept sink_type   = std::invocable<F&, const char*, std::size_t>;
    template<class F> concept source_type = std::invocable<F&, char*, std::size_t>;

//----------------------------------------------------------------------------------------------------------------

}