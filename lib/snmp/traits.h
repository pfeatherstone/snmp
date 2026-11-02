#pragma once

#include <cstdint>
#include <cstddef>
#include <type_traits>
#include <concepts>
#include <iterator>
#include <memory>
#include <utility>
#include <string_view>

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

    namespace detail
    {
        using std::begin;
        using std::end;
        using std::size;
        using std::data;

        template <typename T>
        constexpr auto adl_begin(T&& v) -> decltype(begin(std::forward<T>(v)))
        {
            return begin(std::forward<T>(v));
        }

        template <typename T>
        constexpr auto adl_end(T&& v) -> decltype(end(std::forward<T>(v)))
        {
            return end(std::forward<T>(v));
        }

        template <typename T>
        constexpr auto adl_size(T&& v) -> decltype(size(std::forward<T>(v)))
        {
            return size(std::forward<T>(v));
        }

        template <typename T>
        constexpr auto adl_data(T&& v) -> decltype(data(std::forward<T>(v)))
        {
            return data(std::forward<T>(v));
        }
    }
    
//----------------------------------------------------------------------------------------------------------------

    template<class C>
    concept const_sequence_container = requires(const C& v)
    {
        requires (!std::convertible_to<const C&, std::string_view>);
        { detail::adl_begin(v) } -> std::forward_iterator;
        { detail::adl_end(v)   } -> std::sentinel_for<decltype(detail::adl_begin(v))>;
    };

    template<class C>
    using sequence_value_t = std::iter_value_t<decltype(detail::adl_begin(std::declval<C&>()))>;

//----------------------------------------------------------------------------------------------------------------

    template<class C>
    concept appendable_sequence_container = requires(C& v, sequence_value_t<C>&& element)
    {
        requires (!std::convertible_to<const C&, std::string_view>);
        v.push_back(std::move(element));
        v.clear();
    };

//----------------------------------------------------------------------------------------------------------------

    template<class F> concept sink_type   = std::invocable<F&, const char*, std::size_t>;
    template<class F> concept source_type = std::invocable<F&, char*, std::size_t>;

//----------------------------------------------------------------------------------------------------------------

    template<class C>
    concept sink_buffer = requires(C& buf, const char* first, const char* last)
    {
        buf.insert(detail::adl_end(buf), first, last);
    };

//----------------------------------------------------------------------------------------------------------------

    template<class C>
    concept source_buffer = requires(const C& buf)
    {
        { detail::adl_size(buf) } -> std::convertible_to<std::size_t>;
        requires std::is_pointer_v<decltype(detail::adl_data(buf))>;
        requires is_byte<std::remove_cv_t<std::remove_pointer_t<decltype(detail::adl_data(buf))>>>;
    };

//----------------------------------------------------------------------------------------------------------------

}