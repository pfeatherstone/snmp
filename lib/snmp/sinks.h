#pragma once

#include <cstring>
#include <ostream>
#include <istream>
#include "traits.h"
#include "error.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    auto sink(sink_buffer auto& buf) 
    {
        return [&](const char* bytes, size_t nbytes) {
            buf.insert(detail::adl_end(buf), bytes, bytes + nbytes);
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

    auto source(source_buffer auto& buf)
    {
        size_t offset{0};
        return [&buf, offset](char* bytes, size_t nbytes) mutable {
            if ((detail::adl_size(buf) - offset) < nbytes)
                throw std::system_error(snmp_error::SOURCE_OUT_OF_DATA);
            std::memcpy(bytes, detail::adl_data(buf) + offset, nbytes);
            offset += nbytes;
        };
    } 

//----------------------------------------------------------------------------------------------------------------

    inline auto source(std::istream& in)
    {
        return [&](char* bytes, size_t nbytes) {
            in.read(bytes, nbytes);
            if (in.gcount() != (long)nbytes)
                throw std::system_error(snmp_error::SOURCE_OUT_OF_DATA);
        };
    }

//----------------------------------------------------------------------------------------------------------------

}