#include "brain.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <iostream>
#include <map>
#include <set>

using namespace std;
using namespace ns_aarnn;

class aarnn_Brain_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

// TODO: Implement unit-tests here
TEST_F(aarnn_Brain_Test, Brain_simple_test)
{
    ASSERT_EQ(false, true) << "<NOT IMPLEMENTED>";
}
