#pragma once

#include <random>

namespace snmp_tests
{

//----------------------------------------------------------------------------------------------------------------

    template<class Int, class Generator>
    Int random_int(Generator& gen)
    {
        if constexpr(std::is_same_v<Int, char> || std::is_same_v<Int, int8_t> || std::is_same_v<Int, uint8_t>)
            return std::uniform_int_distribution<int>{std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max()}(gen);
        else
            return std::uniform_int_distribution<Int>{std::numeric_limits<Int>::min(), std::numeric_limits<Int>::max()}(gen);
    }

//----------------------------------------------------------------------------------------------------------------

}
