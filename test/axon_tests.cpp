#include "axon.h"
#include "axon_bouton.h"
#include "axon_branch.h"
#include "axon_hillock.h"

#include <gtest/gtest.h>

using namespace std;
using namespace ns_aarnn;

class aarnn_Axon_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_Axon_Test, Axon_construction_test)
{
    auto axon = Axon::create(Position{1.0, 2.0, 3.0});
    ASSERT_FALSE(axon->isInitialised());
    ASSERT_EQ(axon->getChild<AxonBouton>(), nullptr);
    ASSERT_EQ(axon->getParent<AxonBranch>(), nullptr);
    ASSERT_EQ(axon->getParent<AxonHillock>(), nullptr);
    ASSERT_EQ(axon->getAxonBranches().size(), 0UL);
    axon->initialise();
    ASSERT_TRUE(axon->isInitialised());
    ASSERT_NE(axon->getChild<AxonBouton>(), nullptr);
    ASSERT_EQ(axon->getChild<AxonBouton>()->getParent<Axon>(), axon) << "parent of member AxonBouton should be this Axon";
    ASSERT_EQ(axon->getParent<AxonBranch>(), nullptr);
    axon->setParentAxonBranch(AxonBranch::create(Position{axon->x()+1.0,axon->y()+1.0,axon->y()+1.0}));
    ASSERT_NE(axon->getParent<AxonBranch>(), nullptr);
    ASSERT_EQ(axon->getParent<AxonHillock>(), nullptr);
    axon->setParentAxonHillock(AxonHillock::create(Position{axon->x()-1.0,axon->y()-1.0,axon->y()-1.0}));
    ASSERT_NE(axon->getParent<AxonHillock>(), nullptr);
    ASSERT_EQ(axon->getAxonBranches().size(), 0UL);
    for(size_t i = 0UL; i < 5UL; ++i)
        axon->addBranch(AxonBranch::create(Position{axon->x()-0.1,axon->y()-0.1,axon->y()-0.1}));
    ASSERT_EQ(axon->getAxonBranches().size(), 5UL);
}
