#include "body_component.h"

namespace ns_aarnn
{
std::mutex& BodyComponent::mtx()
{
    static auto m = std::mutex{};
    return m;
}

BodyComponent::BodyComponent(size_t id, const Position& position)
: Position(position.x(), position.y(), position.z())
, id_(id)
{
}

BodyComponent::BodyComponent(const BodyComponent& rhs)
: Position(rhs)
, pNextID_(rhs.pNextID_)
, id_(pNextID_())
, propagationRate_(rhs.propagationRate_)
, lowerStimulationClamp_(rhs.lowerStimulationClamp_)
, upperStimulationClamp_(rhs.upperStimulationClamp_)
{
}

BodyComponent& BodyComponent::operator=(const BodyComponent& rhs)
{
    if(this != &rhs)
    {
        pNextID_ = rhs.pNextID_;
        // id_                    = id_; We keep the ID of this object
        propagationRate_       = rhs.propagationRate_;
        lowerStimulationClamp_ = rhs.lowerStimulationClamp_;
        upperStimulationClamp_ = rhs.upperStimulationClamp_;
    }
    return *this;
}

std::string_view BodyComponent::name()
{
    return std::string_view{"BodyComponent"};
}
void BodyComponent::initialise()
{
    if(!isInitialised())
    {
        doInitialisation_();
        isInitialised_ = true;
    }
}

size_t BodyComponent::getID() const
{
    return id_;
}

bool BodyComponent::receiveStimulation(int8_t stimulus)
{
    // Implement the stimulus logic
    propagationRate_ += double((propagationRate_ * lowerStimulationClamp_) * static_cast<double>(stimulus));

    // TODO: Current implementation clamps propagation outside of a lower and upper limit to 0.0 - verify that
    //       this is correct - shouldn't it return lowerStimulationClamp_ and upperStimulationClamp_ respectively?
    // Clamp propagationRate_ within the range lower and upper boundaries

    // ORIGINAL:
    //        if(propagationRate_ < lowerStimulationClamp_ || propagationRate_ > upperStimulationClamp_)
    //        {
    //            propagationRate_ = 0.0;
    //            return false;
    //        }
    if(propagationRate_ < lowerStimulationClamp_)
    {
        propagationRate_ = lowerStimulationClamp_;
        return false;
    }
    if(propagationRate_ > upperStimulationClamp_)
    {
        propagationRate_ = upperStimulationClamp_;
        return false;
    }
    return true;
}

double BodyComponent::getPropagationRate() const
{
    return propagationRate_;
}

double BodyComponent::getLowerStimulationClamp() const
{
    return lowerStimulationClamp_;
}

double BodyComponent::getUpperStimulationClamp() const
{
    return upperStimulationClamp_;
}

double BodyComponent::calcPropagationTime(const Position& position, double propagationRate)
{
    if(propagationRate <= 0.0 || propagationRate > 1.0)
    {
        std::stringstream ss;
        ss << "propagation rate " << propagationRate << " is outside (0..1]";
        throw body_component_error(__PRETTY_FUNCTION__, ss.str());
    }

    return distanceTo(position) / propagationRate;
}

void BodyComponent::setStimulationClamp(double lowerStimulationClamp, double upperStimulationClamp)
{
    if(lowerStimulationClamp > upperStimulationClamp)
        std::swap(lowerStimulationClamp, upperStimulationClamp);
    if(lowerStimulationClamp < 0.0 || upperStimulationClamp > 1.0 || lowerStimulationClamp == upperStimulationClamp)
    {
        std::stringstream ss;
        ss << "lower and upper clamp need to be different and in interval [0..1], but were: [" << lowerStimulationClamp
           << ".." << upperStimulationClamp << "]";
        throw body_component_error(__PRETTY_FUNCTION__, ss.str());
    }

    lowerStimulationClamp_ = lowerStimulationClamp;
    upperStimulationClamp_ = upperStimulationClamp;
}

void BodyComponent::setNextIDFunction(size_t (*pNextID)())
{
    pNextID_ = pNextID;
}

std::ostream& operator<<(std::ostream& os, const BodyComponent& bodyComponent)
{
    os << "BodyComponent[ID=" << bodyComponent.getID() << " " << static_cast<Position>(bodyComponent)
       << " propagationRate=" << bodyComponent.getPropagationRate() << " limits:[" << bodyComponent.lowerStimulationClamp_ << ".."
       << bodyComponent.upperStimulationClamp_ << "] ]";
    return os;
}
}  // namespace aarnn