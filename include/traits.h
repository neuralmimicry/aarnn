/*
 * Repository:  https://github.com/kingkybel/CPP-utilities
 * File Name:   include/traits.h
 * Description: Macros to define traits that can static_assert whether
 *              static or non static member-functions are present in a type.
 *
 * Copyright (C) 2023 Dieter J Kybelksties <github@kybelksties.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * @date: 2023-08-28
 * @author: Dieter J Kybelksties
 */

#ifndef TRAITS_H_INCLUDED
#define TRAITS_H_INCLUDED

#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <utility>

namespace util
{
template<typename T, typename EqualTo>
struct has_operator_equal_impl
{
    template<typename U, typename V>
    static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
    template<typename, typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template<typename T, typename LessThan>
struct has_operator_less_impl
{
    template<typename U, typename V>
    static auto test(U*) -> decltype(std::declval<U>() < std::declval<V>());
    template<typename, typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, LessThan>(0))>::type;
};

template<typename T, typename EqualTo = T>
struct has_operator_equal : has_operator_equal_impl<T, EqualTo>::type
{
};

template<typename T, typename LessThan = T>
struct has_operator_less : has_operator_less_impl<T, LessThan>::type
{
};

template<typename>
struct is_tuple : std::false_type
{
};

template<typename... T>
struct is_tuple<std::tuple<T...>> : std::true_type
{
};

template<size_t N, typename T, typename... types>
struct get_Nth_type
{
    using type = typename get_Nth_type<N - 1, types...>::type;
};

template<typename T, typename... types>
struct get_Nth_type<0, T, types...>
{
    using type = T;
};

/**
 * @brief Trait to identify custom character types. Default is false.
 */
template<typename>
struct is_char : std::false_type
{
};

/**
 * @brief Trait to identify custom character types. Specialization for "char" true.
 */
template<>
struct is_char<char> : std::true_type
{
};

/**
 * @brief Trait to identify custom character types. Specialization for "wchar_t" true.
 */
template<>
struct is_char<wchar_t> : std::true_type
{
};

/**
 * @brief Trait to identify custom character types. Specialization for "char16_t" true.
 */
template<>
struct is_char<char16_t> : std::true_type
{
};

/**
 * @brief Trait to identify custom character types. Specialization for "char32_t" true.
 */
template<>
struct is_char<char32_t> : std::true_type
{
};

/**
 * @brief Trait to identify std-string types. Default is false.
 */
template<typename>
struct is_std_string : std::false_type
{
    using char_type = void;
};

/**
 * @brief Trait to identify std-string types. Specialization for "std::basic_string" is true.
 */
template<typename CharT, typename Traits, typename Alloc>
struct is_std_string<std::basic_string<CharT, Traits, Alloc>> : std::true_type
{
    using char_type = CharT;
};

/**
 * @brief Convert a char to a different char type
 *
 * @tparam CharTto_ type to convert to
 * @tparam CharTfrom_ type to convert from
 * @param c the char to convert
 * @return constexpr CharTto_ the converted char
 */
template<typename CharTto_,
         typename CharTfrom_,
         typename std::enable_if<util::is_char<CharTto_>::value>::type*   = nullptr,
         typename std::enable_if<util::is_char<CharTfrom_>::value>::type* = nullptr>
constexpr CharTto_ charToChar(CharTfrom_ c)
{
    return static_cast<CharTto_>(c);
}

/**
 * @brief Trait to identify std-string-view types. Default is false.
 */
template<typename U>
struct is_std_string_view : std::false_type
{
    using char_type = void;
};

/**
 * @brief Trait to identify std-string types. Specialization for "std::basic_string_view" is true.
 */
template<typename CharT, typename Traits>
struct is_std_string_view<std::basic_string_view<CharT, Traits>> : std::true_type
{
    using char_type = CharT;
};

/**
 * @brief Trait to identify character pointer types. Default is false.
 */
template<typename U>
struct is_char_pointer : std::false_type
{
    using char_type = void;
};

/**
 * @brief Trait to identify character pointer types. Specialization for "CharT*" is true, if CharT is a character.
 */
template<typename CharT>
struct is_char_pointer<CharT*>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character pointer types. Specialization for "const CharT*" is true, if CharT is a character.
 */
template<typename CharT>
struct is_char_pointer<const CharT*>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character pointer types. Specialization for "const CharT* const" is true, if CharT is a
 * character.
 */
template<typename CharT>
struct is_char_pointer<const CharT* const>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character array types. Default is false.
 */
template<typename U>
struct is_char_array : std::false_type
{
    using char_type = void;
};

/**
 * @brief Trait to identify character array types. Specialization for "CharT[]" is true, if CharT is a character.
 */
template<typename CharT>
struct is_char_array<CharT[]>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character array types. Specialization for "CharT[sz]" is true, if CharT is a character.
 */
template<typename CharT, size_t sz>
struct is_char_array<CharT[sz]>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character array types. Specialization for "const CharT[]" is true, if CharT is a character.
 */
template<typename CharT>
struct is_char_array<const CharT[]>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify character array types. Specialization for "const CharT[sz]" is true, if CharT is a
 * character.
 */
template<typename CharT, size_t sz>
struct is_char_array<const CharT[sz]>
{
    static constexpr bool                                       value = is_char<CharT>::value;
    typedef typename std::conditional<value, CharT, void>::type char_type;
};

/**
 * @brief Trait to identify "logical strings": char* char-arrays, string_views and strings.
 *
 * @tparam T tyep to check
 */
template<typename T>
struct is_string
{
    // Check for custom string-like types
    static constexpr bool is_std_string_v      = is_std_string<T>::value;
    static constexpr bool is_std_string_view_v = is_std_string_view<T>::value;
    static constexpr bool is_char_pointer_v    = is_char_pointer<T>::value;
    static constexpr bool is_char_array_v      = is_char_array<T>::value;

    static constexpr bool value = is_std_string_v || is_std_string_view_v || is_char_pointer_v || is_char_array_v;
    typedef typename is_std_string<T>::char_type      __str_char_type;
    typedef typename is_std_string_view<T>::char_type __str_vw_char_type;
    typedef typename is_char_pointer<T>::char_type    __pchr_char_type;
    typedef typename is_char_array<T>::char_type      __Achr_char_type;

    // clang-format off
    typedef typename std::conditional<is_std_string_v,
                                       __str_char_type,
                                       std::conditional_t<is_std_string_view_v,
                                                          __str_vw_char_type,
                                                          std::conditional_t<is_char_pointer_v,
                                                                             __pchr_char_type,
                                                                             __Achr_char_type>>>::type char_type;
    // clang-format on
};

// clang-format on

// template<typename T>
// struct is_string_type
// : std::disjunction<std::is_convertible<T, std::string>,
//                    std::is_convertible<T, std::wstring>,
//                    std::is_convertible<T, std::u8string>,
//                    std::is_convertible<T, std::u16string>,
//                    std::is_convertible<T, std::u32string>,
//                    std::is_convertible<T, std::string_view>,
//                    std::is_convertible<T, std::wstring_view>,
//                    std::is_convertible<T, std::u8string_view>,
//                    std::is_convertible<T, std::u16string_view>,
//                    std::is_convertible<T, std::u32string_view>,
//                    std::is_convertible<T, const char*>,
//                    std::is_convertible<T, const wchar_t*>,
//                    std::is_convertible<T, const char16_t*>,
//                    std::is_convertible<T, const char32_t*>,
//                    std::is_array<T>,
//                    std::is_same<T, char*>,
//                    std::is_same<T, wchar_t*>,
//                    std::is_same<T, char16_t*>,
//                    std::is_same<T, char32_t*>>
// {
// };

template<typename StringT1_, typename StringT2_>
struct is_compatible_string
{
    static constexpr bool                            is_string_1_v = is_string<StringT1_>::value;
    static constexpr bool                            is_string_2_v = is_string<StringT2_>::value;
    typedef typename is_string<StringT1_>::char_type char_type_1;
    typedef typename is_string<StringT2_>::char_type char_type_2;
    static constexpr bool                            are_same = std::is_same<char_type_1, char_type_2>::value;

    constexpr static bool value = is_string_1_v && is_string_2_v && are_same;
    typedef typename std::conditional<value, char_type_1, void>::type type;
};

template<typename StringT_, typename StringOrCharT_>
struct has_std_string_compatible_char
{
    // clang-format off
    typedef typename is_string<StringT_>::char_type string_char_type;

    static constexpr bool is_std_string_1st_v    = is_std_string<StringT_>::value;
    static constexpr bool is_compatible_string_v = is_compatible_string<StringT_, StringOrCharT_>::value;
    static constexpr bool is_char_2nd_v          = is_char<StringOrCharT_>::value;
    static constexpr bool is_compatible_char_v   = is_char_2nd_v && std::is_same<string_char_type, StringOrCharT_>::value;

    constexpr static bool value                  = is_compatible_string_v || is_compatible_char_v;
    typedef typename std::conditional<value, string_char_type, void>::type type;
    // clang-format on
};

template<typename T>
typename std::enable_if<util::is_std_string<T>::value || util::is_std_string_view<T>::value, size_t>::type
 string_or_char_size(const T& str)
{
    return str.size();
}

template<typename T>
typename std::enable_if<util::is_char_pointer<T>::value || util::is_char_array<T>::value, size_t>::type
 string_or_char_size(const T& str)
{
    typedef typename util::is_string<T>::char_type char_type;
    return std::char_traits<char_type>::length(str);
}

template<typename T>
typename std::enable_if<util::is_char<T>::value, size_t>::type string_or_char_size(const T& str)
{
    return 1UL;
}

template<typename T>
typename std::enable_if<!util::is_string<T>::value && !util::is_char<T>::value, size_t>::type
 string_or_char_size(const T& /* str */)
{
    return 0UL;
}

};  //  namespace util

#endif  // TRAITS_H_INCLUDED
