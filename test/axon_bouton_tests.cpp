#include "axon.h"
#include "axon_bouton.h"
#include "synaptic_gap.h"

#include <gtest/gtest.h>

using namespace std;
using namespace ns_aarnn;

class aarnn_AxonBouton_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_AxonBouton_Test, AxonBouton_construction_test)
{
    auto axonBouton = AxonBouton::create(Position{3.0, 2.0, 1.0});
    ASSERT_FALSE(axonBouton->isInitialised());
    ASSERT_EQ(axonBouton->getParent<Axon>(), nullptr);
    ASSERT_EQ(axonBouton->getChild<SynapticGap>(), nullptr);
    axonBouton->initialise();
    ASSERT_TRUE(axonBouton->isInitialised());
    ASSERT_NE(axonBouton->getChild<SynapticGap>(), nullptr);
}
