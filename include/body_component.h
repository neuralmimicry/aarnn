
#ifndef BODY_COMPONENT_H_INCLUDED
#define BODY_COMPONENT_H_INCLUDED

#include "fwd_decls.h"
#include "type_checks.h"
#include "position.h"
#include "traits_static.h"

#include <exception>
#include <memory>
#include <mutex>
#include <ostream>
#include <sstream>
#include <string>
#include <utility>

namespace ns_aarnn
{
/**
 * @brief Function to return consecutive IDs starting from 0(zero).
 * @note Put in a function in order to avoid static variables in the library.
 *       Template parameter ensures, that each class can have its own sequence.
 *       nextID<Class1>() has a different sequence from nextID<Class2>().
 * @return the next ID
 */
template<typename T_>
inline size_t nextID()
{
    static size_t id_counter = 0UL;
    id_counter++;
    return id_counter - 1;
}

/**
 * @brief Exception class for BodyComponent errors.
 */
struct body_component_error : public std::invalid_argument
{
    explicit body_component_error(const std::string& function, const std::string& message)
    : std::invalid_argument(function + ": " + message)
    {
        spdlog::error(message);
    }
};

/**
 * @brief Abstract base class for all constituent objects of a brain model.
 * Derives from @c Position and thus treating each derived object as a position. This makes sense as one of the fundamental
 * attributes of  \c Neuron s, \c Axon s etc is their position in 3D space, and makes it more obvious when calculating
 * distance between objects for example:
 *
 * @code{.cpp}
 * auto dist = axon.distanceTo(dendrite);
 * @endcode
 */
class BodyComponent
: public std::enable_shared_from_this<BodyComponent>
, public Position
{
    protected:
    /**
     * @brief Mutex to be used by all derived classes.
     * @note this is wrapped in a function, as static global objects are not a good idea in libraries
     * @return the static mutex
     */
    static std::mutex& mtx();

    public:
    template<typename Derived_>
    std::shared_ptr<Derived_> this_shared()
    {
        return std::dynamic_pointer_cast<Derived_>(shared_from_this());
    }

    static constexpr double PROPAGATION_RATE_DEFAULT        = 0.5;
    static constexpr double LOWER_STIMULATION_CLAMP_DEFAULT = 0.1;
    static constexpr double UPPER_STIMULATION_CLAMP_DEFAULT = 0.9;

    /**
     * @brief Constructor.
     * @param id ID of this object
     * @param position position of this object in 3D space
     */
    explicit BodyComponent(size_t id, const Position& position);

    /**
     * @brief Copy constructor. Generates an exact copy with the exception of the ID. The ID will be the next
     * generated ID.
     * @param rhs right-hand-side body-component
     */
    BodyComponent(const BodyComponent& rhs);

    /**
     * @brief Assignment operator. Generates an exact copy with the exception of the ID. The ID will remain the current
     * ID.
     * @param rhs right-hand-side body-component
     * @return this modified object
     */
    BodyComponent& operator=(const BodyComponent& rhs);
    BodyComponent& operator=(BodyComponent&& rhs) noexcept = default;
    BodyComponent(BodyComponent&& rhs)                     = default;
    virtual ~BodyComponent()                               = default;

    // abstract functions

    /**
     * @brief @static name of the class.
     * @note this is not an abstract method, but a static method. However with static asserts using
     *       @c has_static_name_function derived functions are forced to implement such a static function.
     *
     * @return the class name
     */
    [[nodiscard]] static std::string_view name();

    /**
     * @brief abstract function to calculate the propagation rate.
     * Different derived classes will calculate this differently using their respective members.
     * @return the calculated propagation rate
     */
    [[nodiscard]] virtual double calculatePropagationRate() const = 0;

    protected:
    /**
     * @brief abstract initialisation function.
     * Derived classes use this to initialise their members.
     */
    virtual void doInitialisation_() = 0;

    public:
    /**
     * @brief Perform the actual initialisation by calling the protected overridden doInitialisation_() functions.
     */
    void initialise();

    /**
     * @brief Retrieve the (unique) ID of this object.
     * @return the ID
     */
    [[nodiscard]] size_t getID() const;

    /**
     * @brief adjust the stimulus rate depending on the stimulus. Clamp the rate at specified upper and lower limits.
     *
     * @param stimulus the stimulus this body receives
     * @return true, if propagation rate was not clamped, false otherwise
     */
    bool receiveStimulation(int8_t stimulus);

    /**
     * @brief Retrieve the propagation rate. Only retrieving the value and not calculating it.
     * @return the propagation rate
     */
    [[nodiscard]] double getPropagationRate() const;

    /**
     * @brief Retrieve the lower boundary for stimulation.
     * @return the lower stimulation clamp
     */
    [[nodiscard]] double getLowerStimulationClamp() const;

    /**
     * @brief Retrieve the u[[er boundary for stimulation.
     * @return the upper stimulation clamp
     */
    [[nodiscard]] double getUpperStimulationClamp() const;

    /**
     * @brief Calculate the time needed for propagation from the position of this object to the given position with
     * given propagation rate.
     *
     * @param position the position to calculate the time for
     * @param propagationRate the propagation rate to use
     * @return double time it takes to propagate from position to here
     */
    virtual double calcPropagationTime(const Position& position, double propagationRate);

    /**
     * @brief Set the limits where stimulus should be clamped.
     *
     * @param lowerStimulationClamp new lower level
     * @param upperStimulationClamp new higher level
     */
    void setStimulationClamp(double lowerStimulationClamp = LOWER_STIMULATION_CLAMP_DEFAULT,
                             double upperStimulationClamp = UPPER_STIMULATION_CLAMP_DEFAULT);

    /**
     * @brief Check whether this object is initialised or not.
     * @return true, if the object is initialised, false otherwise.
     */
    [[nodiscard]] bool isInitialised() const
    {
        return isInitialised_;
    }

    /**
     * @brief Standard out-stream operator.
     * @param os out-stream to modify
     * @param bodyComponent the component to stream
     * @return the modified stream
     */
    friend std::ostream& operator<<(std::ostream& os, const BodyComponent& bodyComponent);

    /**
     * @brief Get the parent pointer cast to the correct derived class
     * @tparam ParentT_ pointer to parent class object type
     * @return the child if it exists and matches the template-parameter type, null otherwise
     * @throws body_component_error if a child exists but cannot be cast to the given template parameter type
     */
    template<typename ParentT_ = BodyComponent>
    std::shared_ptr<ParentT_> getParent() const
    {
        static_assert(std::is_base_of_v<BodyComponent, ParentT_>, "ParentT_ must be derived from BodyComponent");
        static_assert(has_static_name_function<ParentT_>::value,
                      "ParentT_ must have a static std::string_view name() function");
        auto foundParent = parents_.find(ParentT_::name());
        if(foundParent == parents_.end())
            return nullptr;
        auto reval = std::dynamic_pointer_cast<ParentT_>(foundParent->second);
        if(!reval)  // this should never happen unless name of two incompatible classes are the same
            throw body_component_error(__PRETTY_FUNCTION__, "illegal ParentT_ in dynamic cast");
        return reval;
    }

    /**
     * @brief Get the child pointer cast to the correct derived class
     * @tparam ChildT_ pointer to child class object type
     * @return the child if it exists and matches the template-parameter type, null otherwise
     * @throws body_component_error if a child exists but cannot be cast to the given template parameter type
     */
    template<typename ChildT_ = BodyComponent>
    std::shared_ptr<ChildT_> getChild() const
    {
        static_assert(std::is_base_of_v<BodyComponent, ChildT_>, "ChildT_ must be derived from BodyComponent");
        static_assert(has_static_name_function<ChildT_>::value,
                      "ChildT_ must have a static std::string_view name() function");
        auto foundChild = children_.find(ChildT_::name());
        if(foundChild == children_.end())
            return nullptr;
        auto reval = std::dynamic_pointer_cast<ChildT_>(foundChild->second);
        if(!reval)  // this should never happen unless name of two incompatible classes are the same
            throw body_component_error(__PRETTY_FUNCTION__, "illegal ChildT_ in dynamic cast");
        return reval;
    }

    /**
     * @brief Set a new parent for this object and make this the parent's child.
     * @tparam ParentT_ type of parent object. Must be concrete class derived from {@see BodyComponent}.
     * @tparam ChildT_ type of child object. Must be concrete class derived from {@see BodyComponent}.
     * @param parent parent object
     * @param child child object
     */
    template<typename ParentT_, typename ChildT_>
    friend void connectParentAndChild(std::shared_ptr<ParentT_> parent, std::shared_ptr<ChildT_> child)
    {
        static_assert(std::is_base_of_v<BodyComponent, ParentT_>, "ParentT_ must be derived from BodyComponent");
        static_assert(has_static_name_function<ParentT_>::value,
                      "ParentT_ must have a static std::string_view name() function");
        static_assert(std::is_base_of_v<BodyComponent, ChildT_>, "ChildT_ must be derived from BodyComponent");
        static_assert(has_static_name_function<ChildT_>::value,
                      "ChildT_ must have a static std::string_view name() function");

        if(static_cast<BodyComponent*>(parent.get()) == static_cast<BodyComponent*>(child.get()))
        {
            throw body_component_error(__PRETTY_FUNCTION__, "attempt to set this as its own parent");
        }
        if(!parent)
        {
            throw body_component_error(__PRETTY_FUNCTION__, "cannot connect <null> parent");
        }
        if(!child)
        {
            throw body_component_error(__PRETTY_FUNCTION__, "cannot connect <null> child");
        }

        auto existingChildOfParent = parent->children_.find(ChildT_::name());
        if(existingChildOfParent != parent->children_.end())
        {
            // disconnect original child
            auto body = existingChildOfParent->second;

            body->parents_.erase(ParentT_::name());
            parent->children_.erase(existingChildOfParent);
        }

        auto existingParentOfChild = child->parents_.find(ParentT_::name());
        if(existingParentOfChild != child->parents_.end())
        {
            // disconnect original parent
            existingParentOfChild->second->children_.erase(ChildT_::name());
            child->parents_.erase(existingParentOfChild);
        }

        parent->children_[ChildT_::name()] = child;
        child->parents_[ParentT_::name()]  = parent;
    }

    protected:
    /**
     * @brief Set the generator function for IDs. This generator will be used to create consecutive IDs for objects
     * of derived classes.
     * @param pNextID function pointer to the generator, eg. for class Xyz: nextID<Xyz>
     */
    void setNextIDFunction(size_t (*pNextID)());

    private:
    size_t (*pNextID_)() = nextID<BodyComponent>;
    size_t                                                 id_;
    bool                                                   isInitialised_{false};
    std::unordered_map<std::string_view, BodyComponentPtr> parents_{};
    std::unordered_map<std::string_view, BodyComponentPtr> children_{};

    double propagationRate_{PROPAGATION_RATE_DEFAULT};
    double lowerStimulationClamp_{LOWER_STIMULATION_CLAMP_DEFAULT};
    double upperStimulationClamp_{UPPER_STIMULATION_CLAMP_DEFAULT};
};
}  // namespace aarnn

#endif  // BODY_COMPONENT_H_INCLUDED
