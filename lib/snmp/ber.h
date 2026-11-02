#pragma once

#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include "traits.h"

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

    template<class... Types>
    void ber_serialize(const std::variant<Types...>& v, sink_type auto&& sink);

    template<class... Types>
    void ber_deserialize(std::variant<Types...>& v, source_type auto&& src);

//----------------------------------------------------------------------------------------------------------------

}

#include "ber_impl.h"