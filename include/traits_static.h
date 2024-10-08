/*
 * Repository:  https://github.com/kingkybel/CPP-utilities
 * File Name:   include/traits_static.h
 * Description: Enforce static functions in classes.
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
#ifndef TRAITS_STATIC_H_INCLUDED
#define TRAITS_STATIC_H_INCLUDED

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

namespace util
{

/**
 * @brief Macro to make it a bit easier to define a trait to check whether a class has a static member of a given
 * signature.
 * @usage
 * add this macro to your project with the first parameter the name
 * you want to give the trait and the second the name of the member function
 * whose presence you wish to assert and the third the signature of the static member
 *
 * @code{.cpp}
 * DEFINE_HAS_STATIC_MEMBER(has_static_bool_fill, T::fill, bool (*)(void));
 *
 * struct test1
 * {
 *     static bool fill() { return false; }
 * };
 *
 * struct
 * {
 * static double fill() { return 0.0; } // wrong signature!
 * };
 *
 * cout &lt;&lt; has_static_bool_fill&lt;test1&gt;::value &lt;&lt; endl; // --> 1
 * cout &lt;&lt; has_static_bool_fill&lt;test2&gt;::value &lt;&lt; endl; // --> 0
 * @endcode
 */
#define DEFINE_HAS_STATIC_MEMBER(traitsName, funcName, signature)                  \
    template<typename U>                                                           \
    class traitsName                                                               \
    {                                                                              \
        private:                                                                   \
        template<typename T, T>                                                    \
        struct helper;                                                             \
        template<typename T>                                                       \
        static std::uint8_t check(helper<signature, &funcName> *);                 \
        template<typename T>                                                       \
        static std::uint16_t check(...);                                           \
                                                                                   \
        public:                                                                    \
        static constexpr bool value = sizeof(check<U>(0)) == sizeof(std::uint8_t); \
    }

/**
 * @usage add this macro to your project with the first parameter the name
 * you want to give the trait and the second the name of the member function
 * whose presence you wish to assert
 *
 * @code{.cpp}
 * DEFINE_HAS_MEMBER(has_some_func, some_func)
 *
 * struct test1
 * {
 *     int some_func(std::string s) { return 0; }
 * };
 *
 * struct test2
 * {
 *     double some_func(std::string s) { return 0; } // wrong signature!
 * };
 *
 * cout &lt;&lt; has_some_func &lt;test1, int(std::string)&gt;::value &lt;&lt; endl; // --> 1
 * cout &lt;&lt; has_some_func &lt;test2, int(std::string)&gt;::value &lt;&lt; endl; // --> 0
 *
 * @endcode
 */
#define DEFINE_HAS_MEMBER(TraitName, FunctionName)                                                    \
    template<typename, typename T>                                                                    \
    struct TraitName                                                                                  \
    {                                                                                                 \
        static_assert(std::integral_constant<T, false>::value,                                        \
                      "Second template parameter needs to be of function type.");                     \
    };                                                                                                \
                                                                                                      \
    /* specialization that does the checking */                                                       \
    template<typename C, typename Ret, typename... Args>                                              \
    struct TraitName<C, Ret(Args...)>                                                                 \
    {                                                                                                 \
        private:                                                                                      \
        template<typename T>                                                                          \
        static constexpr auto check(T *) ->                                                           \
         typename std::is_same<decltype(std::declval<T>().FunctionName(std::declval<Args>()...)),     \
                               Ret      /* ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^*/   \
                               >::type; /* attempt to call it and see if the return type is correct*/ \
                                                                                                      \
        template<typename>                                                                            \
        static constexpr std::false_type check(...);                                                  \
                                                                                                      \
        using type = decltype(check<C>(0));                                                           \
                                                                                                      \
        public:                                                                                       \
        static constexpr bool value = type::value;                                                    \
    };

}  // namespace util

#endif  // TRAITS_STATIC_H_INCLUDED
