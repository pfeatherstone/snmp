#pragma once

#include <system_error>
#include <type_traits>

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    enum class snmp_error
    {
        UNEXPECTED_TAG = 1,
        UNEXPECTED_LENGTH,
        LENGTH_HEADER_OVERFLOW,
        INTEGER_OVERFLOW,
        SOURCE_OUT_OF_DATA
    };

    std::error_code make_error_code(snmp_error e);

//----------------------------------------------------------------------------------------------------------------

}

namespace std
{

//----------------------------------------------------------------------------------------------------------------

    template<>
    struct is_error_code_enum<snmp::snmp_error> : std::true_type{};

//----------------------------------------------------------------------------------------------------------------

}