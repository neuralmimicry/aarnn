#ifndef TYPE_CHECKS_H_INCLUDED
#define TYPE_CHECKS_H_INCLUDED

#include "traits_static.h"

#include <string_view>

/**
 * @brief This template defines a static compiler check that a class has a member-function with the following signature:
 *      std::string_view has_static_name_function()
 *      Usage e.g.:
 * @code
 * template<typename ClassX>
 * void someFunc() {
 *      static_assert(has_static_name_function<ClassX>::value, "must have a static std::string_view name() function");
 * }
 * @endcode
 */
DEFINE_HAS_STATIC_MEMBER(has_static_name_function, T::name, std::string_view (*)(void));

#endif  // TYPE_CHECKS_H_INCLUDED
