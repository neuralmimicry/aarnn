#include "thread_safe_queue.h"

#include <gtest/gtest.h>
#include <set>

using namespace std;
using namespace ns_aarnn;

class aarnn_ThreadSafeQueue_Test : public ::testing::Test
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
TEST_F(aarnn_ThreadSafeQueue_Test, ThreadSafeQueue_simple_test)
{
    ASSERT_EQ(false, true) << "<NOT IMPLEMENTED>";
}
