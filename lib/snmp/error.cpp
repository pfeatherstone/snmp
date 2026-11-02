#include <system_error>
#include "error.h"

namespace snmp
{

//----------------------------------------------------------------------------------------------------------------

    struct snmp_error_category : std::error_category
    {
        const char* name() const noexcept override 
        {
            return "snmp_error_category";
        }

        std::string message(int ec) const override
        {
            switch(static_cast<snmp_error>(ec))
            {
            case snmp_error::UNEXPECTED_TAG:
                return "Found unexpected tag";
            case snmp_error::UNEXPECTED_LENGTH:
                return "Read unexpected length field";
            case snmp_error::LENGTH_HEADER_OVERFLOW:
                return "Decoded length exceeds size_t capacity";
            case snmp_error::INTEGER_OVERFLOW:
                return "Decoded integer overlows supplied integer type";
            case snmp_error::SOURCE_OUT_OF_DATA:
                return "Source is out of data";
            default:
                return "Unrecognised error";
            }
        }
    };

    const snmp_error_category snmp_error_category_;

    std::error_code make_error_code(snmp_error e)
    {
        return {static_cast<int>(e), snmp_error_category_};
    }

//----------------------------------------------------------------------------------------------------------------

}