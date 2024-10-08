#include "position.h"

#include <gtest/gtest.h>

using namespace std;
using namespace ns_aarnn;

class aarnn_Position_Test : public ::testing::Test
{
    protected:
    void SetUp() override
    {
    }

    void TearDown() override
    {
    }
};

TEST_F(aarnn_Position_Test, Position_construction_and_move_test)
{
    Position p1{};
    Position p2{1.0};
    Position p3{2.0, 3.0};
    Position p4{4.0, 5.0, 6.0};
    ASSERT_FLOAT_EQ(p1.x(), 0.0);
    ASSERT_FLOAT_EQ(p1.y(), 0.0);
    ASSERT_FLOAT_EQ(p1.z(), 0.0);
    ASSERT_FLOAT_EQ(p4.x(), 4.0);
    ASSERT_FLOAT_EQ(p4.y(), 5.0);
    ASSERT_FLOAT_EQ(p4.z(), 6.0);

    p2.move(-1.0, 0.0, 0.0);
    ASSERT_EQ(p2, p1);
    p3.move(2.0, 2.0, 6.0);
    ASSERT_EQ(p3, p4);

    Position offset{3.0, 2.0, 1.0};
    p4.move(offset);
    Position afterOffset{7.0, 7.0, 7.0};
    ASSERT_EQ(p4, afterOffset);
}

TEST_F(aarnn_Position_Test, Position_moveRelativeTo_test)
{
    {
        Position p1{};
        Position p2{1.0};
        p1.moveRelativeTo(p2, 1.0, 2.0, 3.0);

        //    (0)    (1)      (1)           (2)
        // p1=(0) p2=(0)  rel=(2)      ---> (2)
        //    (0)    (0)      (3)           (3)
        Position afterOffset{2.0, 2.0, 3.0};

        ASSERT_EQ(p1, afterOffset);
    }
    {
        Position p1{1.0, -1.0, 1.0};
        Position p2{1.0, -3.0, 1.0};
        p1.moveRelativeTo(p2, -2.0, 5.0, -1.0);

        //.    (1)    (1)       (-2)          (0)
        // p1=(-1) p2=(-3)  rel=(5)      ---> (1)
        //.    (1)    (1)       (-1)          (1)
        Position afterOffset{0.0, 1.0, 1.0};

        ASSERT_EQ(p1, afterOffset);
    }
}

TEST_F(aarnn_Position_Test, Position_scale_test)
{
    Position p{1.0, 2.0, 3.0};
    p.scale(2.0);
    Position afterScale{2.0, 4.0, 6.0};
    ASSERT_EQ(p, afterScale);
    p.scale(-3.0);
    afterScale = Position{-6.0, -12.0, -18.0};
    ASSERT_EQ(p, afterScale);
    p.scale(-1.0 / 6.0);
    afterScale = Position{1.0, 2.0, 3.0};
    ASSERT_EQ(p, afterScale);
    ASSERT_NO_THROW(p.scale(0.0));
}

TEST_F(aarnn_Position_Test, Position_distanceTo_test)
{
    Position p1{}, p2{};
    ASSERT_FLOAT_EQ(p1.distanceTo(p2), 0.0);

    Position p3{}, p4{1.0, 0.0, 0.0};
    ASSERT_FLOAT_EQ(p3.distanceTo(p4), 1.0);

    Position p5{}, p6{0.0, 2.0, 0.0};
    ASSERT_FLOAT_EQ(p5.distanceTo(p6), 2.0);

    Position p7{}, p8{0.0, 0.0, -3.0};
    ASSERT_FLOAT_EQ(p7.distanceTo(p8), 3.0);

    Position p9{}, p10{1.0, 2.0, -2.0};  // sqrt(1+4+4)
    ASSERT_FLOAT_EQ(p9.distanceTo(p10), 3.0);

    Position p11{2.0, 4.0, 0.0}, p12{1.0, 2.0, -2.0};  // sqrt(1+4+4)
    ASSERT_FLOAT_EQ(p11.distanceTo(p12), 3.0);
}

TEST_F(aarnn_Position_Test, Position_layeredFibonacciSpherePoint_test)
{
    ASSERT_THROW(Position::layeredFibonacciSpherePoint(-1, 2), std::invalid_argument);
    auto p1 = Position::layeredFibonacciSpherePoint(0, 3);
    auto p2 = Position::layeredFibonacciSpherePoint(1, 3);
    auto p3 = Position::layeredFibonacciSpherePoint(2, 3);
    ASSERT_NE(p1, p2);
    ASSERT_NE(p1, p3);
    ASSERT_NE(p2, p3);

    const size_t TOTAL_POINTS = 22UL;
    std::vector<Position> posVec;
    posVec.reserve(TOTAL_POINTS);
    for(size_t i = 0Ul; i < TOTAL_POINTS; ++i)
        posVec.push_back(Position::layeredFibonacciSpherePoint(i, TOTAL_POINTS));

    for(size_t i = 0Ul; i < TOTAL_POINTS - 1UL; ++i)
        for(size_t j = i + 1; j < TOTAL_POINTS; ++j)
            if(j != i)
                ASSERT_NE(posVec[0], posVec[1]) << "i=" << 0 << " j=" << 1;
}