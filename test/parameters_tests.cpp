#include "parameters.h"
#include "neuron.h"
#include "axon.h"

#include <gtest/gtest.h>

using namespace std;
using namespace ns_aarnn;

class aarnn_Parameters_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
        Parameters::reset();
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_Parameters_Test, Parameters_add_and_get_test)
{
    ASSERT_TRUE(Parameters::instance().empty());
    Parameters::instance().set<Neuron>("xyz", 5);
    ASSERT_FALSE(Parameters::instance().empty());
    ASSERT_EQ((Parameters::instance().get<Neuron, int>("xyz")), 5);
    Parameters::instance().set<Neuron>("xyz", 666);
    ASSERT_EQ((Parameters::instance().get<Neuron, int>("xyz")), 666);
    ASSERT_THROW((Parameters::instance().get<Axon, int>("xyz")), parameter_error);
    Parameters::instance().set<Axon>("xyz", 4711.0);
    ASSERT_EQ((Parameters::instance().get<Axon, double>("xyz")), 4711.0);
    ASSERT_EQ((Parameters::instance().get<Neuron, int>("xyz")), 666);

    Parameters::instance().reset();
    ASSERT_TRUE(Parameters::instance().empty());

    Parameters::instance()
     .set<Neuron>("abc", 1)
     .set<Axon>("def", 'a')
     .set<Axon>("ghi", -0.123);

    ASSERT_EQ((Parameters::instance().get<Neuron, int>("abc")), 1);
    ASSERT_EQ((Parameters::instance().get<Axon, char>("def")), 'a');
    ASSERT_EQ((Parameters::instance().get<Axon, double>("ghi")), -0.123);
}
