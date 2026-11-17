#include <gtest/gtest.h>

#include <cmath>
#include <limits>

#include "dojo/moving_average.hpp"

TEST(MovingAverage, SlidingWindow) {
    dojo::MovingAverage average(2);
    EXPECT_FALSE(average.average().has_value());
    average.push(2);
    average.push(4);
    ASSERT_TRUE(average.average().has_value());
    EXPECT_NEAR(average.average().value(), 3.0, 1e-12);
    average.push(8);
    EXPECT_EQ(average.count(), 2u);
    EXPECT_NEAR(average.average().value(), 6.0, 1e-12);
}

TEST(MovingAverage, RejectsZeroWindow) {
    EXPECT_THROW(dojo::MovingAverage(0), std::invalid_argument);
}

TEST(MovingAverage, RejectsNonFiniteInput) {
    dojo::MovingAverage average(3);
    EXPECT_THROW(average.push(std::numeric_limits<double>::infinity()), std::invalid_argument);
    EXPECT_THROW(average.push(std::numeric_limits<double>::quiet_NaN()), std::invalid_argument);
    EXPECT_EQ(average.count(), 0u);
}

TEST(MovingAverage, WindowOneTracksLatestValue) {
    dojo::MovingAverage average(1);
    average.push(5);
    EXPECT_NEAR(average.average().value(), 5.0, 1e-12);
    average.push(-3);
    EXPECT_NEAR(average.average().value(), -3.0, 1e-12);
    EXPECT_EQ(average.count(), 1u);
}

TEST(MovingAverage, HandlesNegativeValues) {
    dojo::MovingAverage average(4);
    average.push(-2);
    average.push(4);
    EXPECT_NEAR(average.average().value(), 1.0, 1e-12);
}
