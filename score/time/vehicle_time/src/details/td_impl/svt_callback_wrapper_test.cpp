/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#include "score/time/vehicle_time/src/details/td_impl/svt_callback_wrapper.h"

#include <score/callback.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <cstdint>
#include <future>
#include <thread>
#include <vector>

namespace score
{
namespace time
{
namespace detail
{
namespace
{

using TestCallback = score::cpp::callback<void(const int&), 64U>;
using TestSvtCallbackWrapper = SvtCallbackWrapper<TestCallback, int>;

TEST(SvtCallbackWrapperTest, InvokeIfChangedReturnsFalseWhenNoCallbackIsSet)
{
    TestSvtCallbackWrapper sut;
    EXPECT_FALSE(sut.IsSet());
    EXPECT_FALSE(sut.InvokeIfChanged(1, 1));
}

TEST(SvtCallbackWrapperTest, InvokeIfChangedCallsStoredCallbackWithArgumentOnEveryNewKey)
{
    TestSvtCallbackWrapper sut;
    std::vector<int> received;
    sut.Set([&received](const int& value) {
        received.push_back(value);
    });

    EXPECT_TRUE(sut.IsSet());
    EXPECT_TRUE(sut.InvokeIfChanged(7, 70));
    EXPECT_TRUE(sut.InvokeIfChanged(8, 80));
    EXPECT_EQ(received, (std::vector<int>{70, 80}));
}

TEST(SvtCallbackWrapperTest, InvokeIfChangedSkipsRepeatedKey)
{
    TestSvtCallbackWrapper sut;
    int invocations{0};
    sut.Set([&invocations](const int&) {
        ++invocations;
    });

    EXPECT_TRUE(sut.InvokeIfChanged(7, 7));
    EXPECT_FALSE(sut.InvokeIfChanged(7, 7));
    EXPECT_FALSE(sut.InvokeIfChanged(7, 8));
    EXPECT_TRUE(sut.InvokeIfChanged(9, 9));
    EXPECT_EQ(invocations, 2);
}

TEST(SvtCallbackWrapperTest, SetForgetsLastKeySoNewCallbackIsInvokedWithUnchangedKey)
{
    TestSvtCallbackWrapper sut;
    sut.Set([](const int&) {});
    EXPECT_TRUE(sut.InvokeIfChanged(7, 7));
    EXPECT_FALSE(sut.InvokeIfChanged(7, 7));

    int replacement_invocations{0};
    sut.Set([&replacement_invocations](const int&) {
        ++replacement_invocations;
    });
    EXPECT_TRUE(sut.InvokeIfChanged(7, 7));
    EXPECT_EQ(replacement_invocations, 1);
}

TEST(SvtCallbackWrapperTest, UnsetRemovesCallbackAndForgetsLastKey)
{
    TestSvtCallbackWrapper sut;
    int invocations{0};
    sut.Set([&invocations](const int&) {
        ++invocations;
    });
    EXPECT_TRUE(sut.InvokeIfChanged(7, 7));
    sut.Unset();

    EXPECT_FALSE(sut.IsSet());
    EXPECT_FALSE(sut.InvokeIfChanged(7, 7));

    sut.Set([&invocations](const int&) {
        ++invocations;
    });
    EXPECT_TRUE(sut.InvokeIfChanged(7, 7));
    EXPECT_EQ(invocations, 2);
}

TEST(SvtCallbackWrapperTest, SettingEmptyCallbackBehavesLikeUnset)
{
    TestSvtCallbackWrapper sut;
    sut.Set([](const int&) {});
    sut.Set(TestCallback{});

    EXPECT_FALSE(sut.IsSet());
    EXPECT_FALSE(sut.InvokeIfChanged(0, 0));
}

TEST(SvtCallbackWrapperTest, UnsetFromWithinCallbackDoesNotDeadlockAndTakesEffectAfterwards)
{
    TestSvtCallbackWrapper sut;
    int invocations{0};
    sut.Set([&sut, &invocations](const int&) {
        ++invocations;
        sut.Unset();
    });

    EXPECT_TRUE(sut.InvokeIfChanged(0, 0));
    EXPECT_FALSE(sut.IsSet());
    EXPECT_FALSE(sut.InvokeIfChanged(0, 0));
    EXPECT_EQ(invocations, 1);
}

TEST(SvtCallbackWrapperTest, SetFromWithinCallbackReplacesCallbackForNextInvocation)
{
    TestSvtCallbackWrapper sut;
    std::vector<int> trace;
    sut.Set([&sut, &trace](const int&) {
        trace.push_back(1);
        sut.Set([&trace](const int&) {
            trace.push_back(2);
        });
    });

    EXPECT_TRUE(sut.InvokeIfChanged(0, 0));
    EXPECT_TRUE(sut.InvokeIfChanged(0, 0));
    EXPECT_EQ(trace, (std::vector<int>{1, 2}));
}

TEST(SvtCallbackWrapperTest, UnsetFromAnotherThreadBlocksUntilInFlightInvocationReturns)
{
    TestSvtCallbackWrapper sut;
    std::promise<void> callback_entered;
    std::promise<void> release_callback;
    auto release_future = release_callback.get_future().share();
    sut.Set([&callback_entered, release_future](const int&) {
        callback_entered.set_value();
        release_future.wait();
    });

    std::thread invoker{[&sut]() {
        std::ignore = sut.InvokeIfChanged(0, 0);
    }};
    callback_entered.get_future().wait();

    auto unset_done = std::async(std::launch::async, [&sut]() {
        sut.Unset();
    });
    EXPECT_EQ(unset_done.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

    release_callback.set_value();
    EXPECT_EQ(unset_done.wait_for(std::chrono::seconds{5}), std::future_status::ready);
    invoker.join();
    EXPECT_FALSE(sut.IsSet());
}

TEST(SvtCallbackWrapperTest, SetFromAnotherThreadBlocksUntilInFlightInvocationReturns)
{
    TestSvtCallbackWrapper sut;
    std::promise<void> callback_entered;
    std::promise<void> release_callback;
    auto release_future = release_callback.get_future().share();
    sut.Set([&callback_entered, release_future](const int&) {
        callback_entered.set_value();
        release_future.wait();
    });

    std::thread invoker{[&sut]() {
        std::ignore = sut.InvokeIfChanged(0, 0);
    }};
    callback_entered.get_future().wait();

    int replacement_invocations{0};
    auto set_done = std::async(std::launch::async, [&sut, &replacement_invocations]() {
        sut.Set([&replacement_invocations](const int&) {
            ++replacement_invocations;
        });
    });
    EXPECT_EQ(set_done.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

    release_callback.set_value();
    EXPECT_EQ(set_done.wait_for(std::chrono::seconds{5}), std::future_status::ready);
    invoker.join();

    EXPECT_TRUE(sut.InvokeIfChanged(0, 0));
    EXPECT_EQ(replacement_invocations, 1);
}

}  // namespace
}  // namespace detail
}  // namespace time
}  // namespace score
