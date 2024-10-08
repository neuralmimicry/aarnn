#ifndef PARAMETERS_H_INCLUDED
#define PARAMETERS_H_INCLUDED

#include "type_checks.h"

#include <any>
#include <map>
#include <spdlog/spdlog.h>
#include <sstream>
#include <stdexcept>
#include <string>

namespace ns_aarnn
{

/**
 * @brief Exception class for \c Parameters errors.
 */
struct parameter_error : public std::runtime_error
{
    explicit parameter_error(const std::string& function, const std::string& message)
    : std::runtime_error(function + ": " + message)
    {
        spdlog::error(message);
    }
};

/**
 * @brief Parameters - singleton for the keeping parameter as key-value pairs
 */
class Parameters
{
    private:
    Parameters() = default;

    public:
    /**
     * @brief Retrieve the only instance of the Parameters object.
     * @note This is wrapped in a function call as this prevents multiple instantiations when the library is included
     *       multiple times.
     * @note This makes it less testable.
     * @return the singleton
     */
    static Parameters& instance()
    {
        static Parameters params{};
        return params;
    }

    /**
     * @brief Clear all values.
     */
    static void reset()
    {
        instance().values_.clear();
    }

    bool empty() const noexcept
    {
        return values_.empty();
    }

    /**
     * @brief Add a parameter to the instance
     * @tparam T_ type for which a parameter is to be added. Type has to satisfy  @c has_static_name_function
     * @tparam ValueT_ type of the value to set
     * @param name name/key of the value
     * @param value the value to set
     * @return reference to the modified Parameters instance
     */
    template<typename T_, typename ValueT_>
    Parameters& set(const std::string& name, ValueT_ value)
    {
        static_assert(has_static_name_function<T_>::value, "T_ must have a static std::string_view name() function");
        const auto key = std::string{T_::name()} + std::string(".") + name;
        values_[key]   = std::any(value);

        return *this;
    }

    /**
     *
     * @tparam T_ type for which a parameter is to be retrieved. Type has to satisfy  @c has_static_name_function
     * @tparam ValueT_ type of the value to set
     * @param name name/key of the value
     * @return the value if exists and value can be cast to ValueT_
     * @throws @c parameter_error if either the key does not exist, or the value cannot be cast to ValueT_
     */
    template<typename T_, typename ValueT_>
    ValueT_ get(const std::string& name) const
    {
        static_assert(has_static_name_function<T_>::value, "T_ must have a static std::string_view name() function");
        const auto key   = std::string{T_::name()} + std::string(".") + name;
        auto       found = values_.find(key);
        if(found == values_.end())
        {
            std::stringstream ss;
            ss << "cannot find value for key '" << key << "'";
            throw parameter_error(__PRETTY_FUNCTION__, ss.str());
        }
        try
        {
            return std::any_cast<ValueT_>(found->second);
        }
        catch(std::exception& e)
        {
            std::stringstream ss;
            ss << "found value for key '" << name << "', but cannot be casted to '" << typeid(ValueT_).name() << "'";
            throw parameter_error(__PRETTY_FUNCTION__, ss.str());
        }
    }

    private:
    std::map<std::string, std::any> values_;
};

}  // namespace ns_aarnn
#endif  // PARAMETERS_H_INCLUDED
