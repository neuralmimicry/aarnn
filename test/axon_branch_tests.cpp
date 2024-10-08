#include "axon_branch.h"

#include <gtest/gtest.h>

using namespace std;
using namespace ns_aarnn;

class aarnn_AxonBranch_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_AxonBranch_Test, AxonBranch_construction_test)
{
    auto axonBranch = AxonBranch::create(Position{3.0, 2.0, 1.0});
    ASSERT_FALSE(axonBranch->isInitialised());
    ASSERT_EQ(axonBranch->getParent<Axon>(), nullptr);
    ASSERT_EQ(axonBranch->getAxons().size(), 0UL);
    axonBranch->initialise();
    ASSERT_TRUE(axonBranch->isInitialised());
    ASSERT_EQ(axonBranch->getAxons().size(), 1UL);
    auto onwardAxon = Axon::create(Position{0.0,2.0,1.0});
    axonBranch->addAxon(onwardAxon);
    ASSERT_EQ(axonBranch->getAxons().size(), 2UL);
}
