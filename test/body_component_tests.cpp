#include "body_component.h"
// #define DO_TRACE_
#include "traceutil.h"

#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string_view>
#include <utility>

using namespace std;
using namespace ns_aarnn;

class TestBody1 : public BodyComponent
{
    public:
    double val_;
    class TestBody2;
    TestBody1(const Position& position, size_t val)
    : BodyComponent(nextID<TestBody1>(), position)
    , val_(static_cast<double>(val))
    {
        setNextIDFunction(nextID<TestBody1>);
    }
    static std::shared_ptr<TestBody1> create(const Position& position, size_t val)
    {
        return std::make_shared<TestBody1>(position, val);
    }
    [[nodiscard]] static std::string_view name()
    {
        return std::string_view{"TestBody1"};
    }
    void doInitialisation_() override
    {
    }

    [[nodiscard]] double calculatePropagationRate() const override
    {
        return static_cast<double>(getID()) * val_;
    }
};

class TestBody2 : public BodyComponent
{
    public:
    double val_;

    TestBody2(const Position& position, size_t val)
    : BodyComponent(nextID<TestBody2>(), position)
    , val_(static_cast<double>(val))
    {
        setNextIDFunction(nextID<TestBody2>);
    }
    static std::shared_ptr<TestBody2> create(const Position& position, size_t val)
    {
        return std::make_shared<TestBody2>(position, val);
    }
    [[nodiscard]] static std::string_view name()
    {
        return std::string_view{"TestBody2"};
    }
    void doInitialisation_() override
    {
    }

    [[nodiscard]] double calculatePropagationRate() const override
    {
        return static_cast<double>(getID()) * val_;
    }
};

class aarnn_BodyComponent_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_BodyComponent_Test, BodyComponent_construction_test)
{
    class TestBody1 b1_1
    {
        Position{1.0, 2.0, 3.0}, 4711
    };
    class TestBody1 b1_2
    {
        Position{2.0, 4.0, 5.0}, 1234
    };
    ASSERT_NE(b1_1.getID(), b1_2.getID());
    ASSERT_EQ(b1_1.getPropagationRate(), BodyComponent::PROPAGATION_RATE_DEFAULT);
    ASSERT_EQ(b1_2.getPropagationRate(), BodyComponent::PROPAGATION_RATE_DEFAULT);

    class TestBody2 b2_1
    {
        Position{1.0, 2.0, 3.0}, 4711
    };
    class TestBody2 b2_2
    {
        Position{2.0, 4.0, 5.0}, 1234
    };
    ASSERT_NE(b2_1.getID(), b2_2.getID());
    ASSERT_EQ(b2_1.getPropagationRate(), BodyComponent::PROPAGATION_RATE_DEFAULT);
    ASSERT_EQ(b2_2.getPropagationRate(), BodyComponent::PROPAGATION_RATE_DEFAULT);

    // check that each class has it's own ascending numbering ID's
    ASSERT_EQ(b1_1.getID(), b2_1.getID());
    ASSERT_EQ(b1_2.getID(), b2_2.getID());
}

TEST_F(aarnn_BodyComponent_Test, BodyComponent_connect_test)
{
    auto b1 = TestBody1::create(Position{1.0, 2.0, 3.0}, 3);
    auto b2 = TestBody2::create(Position{4.0, 2.0, 1.0}, 5);

    ASSERT_EQ(b1->getChild(), nullptr);
    ASSERT_EQ(b1->getParent(), nullptr);
    ASSERT_EQ(b2->getChild(), nullptr);
    ASSERT_EQ(b2->getParent(), nullptr);

    connectParentAndChild(b1, b2);
    ASSERT_EQ(b1->getChild<TestBody2>(), b2);
    ASSERT_EQ(b2->getParent<TestBody1>(), b1);
    ASSERT_EQ(b1->getParent(), nullptr);
    ASSERT_EQ(b2->getChild(), nullptr);

    connectParentAndChild(b2, b1);
    ASSERT_EQ(b1->getChild<TestBody2>(), b2);
    ASSERT_EQ(b2->getParent<TestBody1>(), b1);
    ASSERT_EQ(b1->getParent<TestBody2>(), b2);
    ASSERT_EQ(b2->getChild<TestBody1>(), b1);

    ASSERT_NO_THROW(b1->getChild<TestBody2>());
}

TEST_F(aarnn_BodyComponent_Test, BodyComponent_receiveStimulation_test)
{
    {
        class TestBody1 b1_1
        {
            Position{1.0, 2.0, 3.0}, 4711
        };
        for(size_t i = 0UL; i < 20UL; ++i)
        {
            TRACE1(b1_1)
            auto previousPropagationRate = b1_1.getPropagationRate();
            auto updated                 = b1_1.receiveStimulation(5);
            ASSERT_GE(b1_1.getPropagationRate(), 0.0);
            ASSERT_LE(b1_1.getPropagationRate(), 1.0);
            if(updated)
                ASSERT_GT(b1_1.getPropagationRate(), previousPropagationRate)
                 << "stimulation number " << i << " with 5";
            else
                ASSERT_FLOAT_EQ(b1_1.getPropagationRate(), b1_1.getUpperStimulationClamp());
        }
    }
    {
        class TestBody2 b2_1
        {
            Position{1.0, 2.0, 3.0}, 4711
        };
        for(size_t i = 0UL; i < 20UL; ++i)
        {
            TRACE1(b2_1)
            auto previousPropagationRate = b2_1.getPropagationRate();
            auto updated                 = b2_1.receiveStimulation(-5);
            ASSERT_GE(b2_1.getPropagationRate(), 0.0);
            ASSERT_LE(b2_1.getPropagationRate(), 1.0);
            if(updated)
                ASSERT_LT(b2_1.getPropagationRate(), previousPropagationRate)
                 << "stimulation number " << i << " with -5";
            else
                ASSERT_FLOAT_EQ(b2_1.getPropagationRate(), b2_1.getLowerStimulationClamp());
        }
    }
}

TEST_F(aarnn_BodyComponent_Test, BodyComponent_setStimulationClamp_test)
{
    class TestBody2 b2
    {
        Position{1.0, 2.0, 3.0}, 4711
    };
    b2.setStimulationClamp(0.3, 0.7);
    ASSERT_THROW(b2.setStimulationClamp(-0.5, 0.2), std::invalid_argument);
    ASSERT_THROW(b2.setStimulationClamp(0.5, 1.2), std::invalid_argument);
    ASSERT_THROW(b2.setStimulationClamp(-0.5, 1.2), std::invalid_argument);

    while(b2.getPropagationRate() < b2.getUpperStimulationClamp())
        b2.receiveStimulation(2);
    ASSERT_FLOAT_EQ(b2.getPropagationRate(), b2.getUpperStimulationClamp());
    b2.setStimulationClamp(0.3, 0.8);
    ASSERT_NE(b2.getPropagationRate(), b2.getUpperStimulationClamp());
    while(b2.getPropagationRate() < b2.getUpperStimulationClamp())
        b2.receiveStimulation(2);
    ASSERT_FLOAT_EQ(b2.getPropagationRate(), b2.getUpperStimulationClamp());

    while(b2.getPropagationRate() > b2.getLowerStimulationClamp())
        b2.receiveStimulation(-2);
    ASSERT_FLOAT_EQ(b2.getPropagationRate(), b2.getLowerStimulationClamp());
    b2.setStimulationClamp(0.1, 0.8);
    ASSERT_NE(b2.getPropagationRate(), b2.getLowerStimulationClamp());
    while(b2.getPropagationRate() > b2.getLowerStimulationClamp())
        b2.receiveStimulation(-2);
    ASSERT_FLOAT_EQ(b2.getPropagationRate(), b2.getLowerStimulationClamp());
}

TEST_F(aarnn_BodyComponent_Test, BodyComponent_calcPropagationTime_test)
{
    class TestBody1 b1
    {
        Position{1.0, 2.0, 3.0}, 4711
    };

    class TestBody2 b2
    {
        Position{1.0, 1.0, 0.0}, 1701
    };

    ASSERT_THROW(b1.calcPropagationTime(b2, 0.0), body_component_error);
    ASSERT_THROW(b2.calcPropagationTime(b1, -0.5), body_component_error);
    ASSERT_THROW(b2.calcPropagationTime(b2, 1.5), body_component_error);

    ASSERT_NO_THROW(b1.calcPropagationTime(b2, 0.1));
    ASSERT_NO_THROW(b2.calcPropagationTime(b1, 0.5));
    ASSERT_NO_THROW(b2.calcPropagationTime(b2, 0.7));
}