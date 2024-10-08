#include "dendrite_bouton.h"
#include "synaptic_gap.h"

#include <gtest/gtest.h>
#include <set>

using namespace std;
using namespace ns_aarnn;

class aarnn_DendriteBouton_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_DendriteBouton_Test, DendriteBouton_simple_test)
{
    auto dendriteBouton = DendriteBouton::create(Position{1.0, 2.0, 3.0});
    ASSERT_FALSE(dendriteBouton->isInitialised());
    ASSERT_EQ(dendriteBouton->getChild<SynapticGap>(), nullptr);
    dendriteBouton->initialise();
    ASSERT_TRUE(dendriteBouton->isInitialised());
    // TODO: the initialisation currently does nothing, which means the child Synaptic gap is not set
    dendriteBouton->addSynapticGap(SynapticGap::create(Position{1.1,2.2,3.3}));
    ASSERT_NE(dendriteBouton->getChild<SynapticGap>(), nullptr);
}
