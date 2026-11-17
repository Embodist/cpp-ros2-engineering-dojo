#include <gtest/gtest.h>

#include <stdexcept>
#include <utility>

#include "dojo/scope_exit.hpp"

TEST(ScopeExit, ExecutesOnce) {
    int n = 0;
    {
        dojo::ScopeExit guard([&] { ++n; });
    }
    EXPECT_EQ(n, 1);
}

TEST(ScopeExit, ReleaseDisablesCallback) {
    int n = 0;
    {
        dojo::ScopeExit guard([&] { ++n; });
        guard.release();
    }
    EXPECT_EQ(n, 0);
}

TEST(ScopeExit, MoveTransfersOwnership) {
    int n = 0;
    {
        dojo::ScopeExit source([&] { ++n; });
        dojo::ScopeExit target(std::move(source));
    }
    EXPECT_EQ(n, 1);
}

TEST(ScopeExit, ExceptionUnwinding) {
    int n = 0;
    try {
        dojo::ScopeExit guard([&] { ++n; });
        throw std::runtime_error("x");
    } catch (const std::runtime_error&) {
    }
    EXPECT_EQ(n, 1);
}

TEST(ScopeExit, MoveAssignmentDisposesPrevious) {
    int a = 0;
    int b = 0;
    {
        dojo::ScopeExit old_guard([&] { ++a; });
        dojo::ScopeExit current([&] { ++b; });
        current = std::move(old_guard);
        EXPECT_EQ(b, 1);
    }
    EXPECT_EQ(a, 1);
    EXPECT_EQ(b, 1);
}

TEST(ScopeExit, SelfMoveKeepsSingleCleanup) {
    int n = 0;
    {
        dojo::ScopeExit guard([&] { ++n; });
        dojo::ScopeExit& alias = guard;
        guard = std::move(alias);
    }
    EXPECT_EQ(n, 1);
}

TEST(ScopeExit, MoveFromReleasedIsInert) {
    int n = 0;
    {
        dojo::ScopeExit source([&] { ++n; });
        source.release();
        dojo::ScopeExit target(std::move(source));
    }
    EXPECT_EQ(n, 0);
}
