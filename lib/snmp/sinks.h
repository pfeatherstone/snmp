#pragma once

#include "ber.h"

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
        };
    }

//----------------------------------------------------------------------------------------------------------------

}