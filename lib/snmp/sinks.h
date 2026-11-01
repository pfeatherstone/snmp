#pragma once

#include <ostream>
#include <vector>
#include "traits.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    template<byte_type Byte, class Alloc>
    auto sink(std::vector<Byte, Alloc>& buf)
    {
        return [&](const char* bytes, size_t nbytes) {
            buf.insert(end(buf), bytes, bytes + nbytes);
        };
    } 

//----------------------------------------------------------------------------------------------------------------

    inline auto sink(std::ostream& out)
    {
        return [&](const char* bytes, size_t nbytes) {
            out.write(bytes, nbytes);
            if (!out)
                throw std::ios_base::failure("SNMP sink write failed");
        };
    }

//----------------------------------------------------------------------------------------------------------------

}