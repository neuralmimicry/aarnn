#ifndef STRING_UTILS_H_INCLUDED
#define STRING_UTILS_H_INCLUDED

#include <string>
#include <sstream>
#include <exception>

namespace ns_aarnn
{
struct string_util_runtime_error : public std::runtime_error
{
    explicit string_util_runtime_error(const std::string& function, const std::string& message)
    : std::runtime_error(function + ": " + message)
    {
        spdlog::error(message);
    }
};


template<typename Type_>
inline std::string toString(const Type_& obj)
{
    std::stringstream ss;
    ss << obj;
    return ss.str();
}

inline bool convertStringToBool(const std::string& value)
{
    // Convert to lowercase for case-insensitive comparison
    std::string lowerValue = value;
    std::transform(lowerValue.begin(), lowerValue.end(), lowerValue.begin(), ::tolower);

    if(lowerValue == "true" || lowerValue == "yes" || lowerValue == "1")
    {
        return true;
    }
    else if(lowerValue == "false" || lowerValue == "no" || lowerValue == "0")
    {
        return false;
    }
    else
    {
        // Handle error or throw an exception
        std::stringstream ss;
        ss << "Invalid boolean string representation: " << value;
        throw string_util_runtime_error{__PRETTY_FUNCTION__, ss.str()};
    }
}
}  // namespace aarnn

#endif  // STRING_UTILS_H_INCLUDED
