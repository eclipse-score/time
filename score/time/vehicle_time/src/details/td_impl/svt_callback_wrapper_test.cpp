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

#include "score/time/vehicle_time/src/details/td_impl/svt_test_helpers.h"

#include <score/callback.hpp>

#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>

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
using score::time::test_helpers::CallbackRecorder;

TEST(SvtCallbackWrapperTest, TryToEnvokeDoesNotRememberDataWhenNoCallbackIsSet)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;

    sut.TryToEnvoke(1);
    EXPECT_FALSE(sut.IsSet());

    sut.Set(recorder.Callback());
    sut.TryToEnvoke(1);
    EXPECT_EQ(recorder.Count(), 1U);
    EXPECT_EQ(recorder.Last(), 1);
}

TEST(SvtCallbackWrapperTest, TryToEnvokeCallsStoredCallbackWithDataOnEveryChange)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;
    sut.Set(recorder.Callback());

    EXPECT_TRUE(sut.IsSet());
    sut.TryToEnvoke(7);
    EXPECT_EQ(recorder.Count(), 1U);
    EXPECT_EQ(recorder.Last(), 7);
    sut.TryToEnvoke(8);
    EXPECT_EQ(recorder.Count(), 2U);
    EXPECT_EQ(recorder.Last(), 8);
}

TEST(SvtCallbackWrapperTest, TryToEnvokeSkipsRepeatedData)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;
    sut.Set(recorder.Callback());

    sut.TryToEnvoke(7);
    sut.TryToEnvoke(7);
    EXPECT_EQ(recorder.Count(), 1U);
    EXPECT_EQ(recorder.Last(), 7);
    sut.TryToEnvoke(9);
    EXPECT_EQ(recorder.Count(), 2U);
    EXPECT_EQ(recorder.Last(), 9);
}

TEST(SvtCallbackWrapperTest, SetForgetsLastDataSoNewCallbackIsInvokedWithUnchangedData)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> first;
    sut.Set(first.Callback());
    sut.TryToEnvoke(7);
    sut.TryToEnvoke(7);
    EXPECT_EQ(first.Count(), 1U);
    EXPECT_EQ(first.Last(), 7);

    CallbackRecorder<int> replacement;
    sut.Set(replacement.Callback());
    sut.TryToEnvoke(7);
    EXPECT_EQ(first.Count(), 1U);
    EXPECT_EQ(replacement.Count(), 1U);
    EXPECT_EQ(replacement.Last(), 7);
}

TEST(SvtCallbackWrapperTest, UnsetRemovesCallbackAndForgetsLastData)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;
    sut.Set(recorder.Callback());
    sut.TryToEnvoke(7);
    sut.Unset();

    EXPECT_FALSE(sut.IsSet());
    sut.TryToEnvoke(7);
    EXPECT_EQ(recorder.Count(), 1U);

    sut.Set(recorder.Callback());
    sut.TryToEnvoke(7);
    EXPECT_EQ(recorder.Count(), 2U);
    EXPECT_EQ(recorder.Last(), 7);
}

TEST(SvtCallbackWrapperTest, SettingEmptyCallbackBehavesLikeUnset)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;
    sut.Set(recorder.Callback());
    sut.TryToEnvoke(0);
    sut.Set(TestCallback{});

    EXPECT_FALSE(sut.IsSet());
    sut.TryToEnvoke(0);
    EXPECT_EQ(recorder.Count(), 1U);

    sut.Set(recorder.Callback());
    sut.TryToEnvoke(0);
    EXPECT_EQ(recorder.Count(), 2U);
    EXPECT_EQ(recorder.Last(), 0);
}

TEST(SvtCallbackWrapperTest, UnsetFromWithinCallbackDoesNotDeadlockAndTakesEffectAfterwards)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> recorder;
    sut.Set([&sut, &recorder](const int& value) {
        recorder.Record(value);
        sut.Unset();
    });

    sut.TryToEnvoke(0);
    EXPECT_FALSE(sut.IsSet());
    sut.TryToEnvoke(0);
    EXPECT_EQ(recorder.Count(), 1U);
    EXPECT_EQ(recorder.Last(), 0);
}

TEST(SvtCallbackWrapperTest, SetFromWithinCallbackReplacesCallbackForNextInvocation)
{
    TestSvtCallbackWrapper sut;
    CallbackRecorder<int> first;
    CallbackRecorder<int> second;
    sut.Set([&sut, &first, &second](const int& value) {
        first.Record(value);
        sut.Set(second.Callback());
    });

    sut.TryToEnvoke(0);
    sut.TryToEnvoke(0);
    EXPECT_EQ(first.Count(), 1U);
    EXPECT_EQ(second.Count(), 1U);
    EXPECT_EQ(first.Last(), 0);
    EXPECT_EQ(second.Last(), 0);
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
        sut.TryToEnvoke(0);
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
        sut.TryToEnvoke(0);
    }};
    callback_entered.get_future().wait();

    CallbackRecorder<int> replacement;
    auto set_done = std::async(std::launch::async, [&sut, &replacement]() {
        sut.Set(replacement.Callback());
    });
    EXPECT_EQ(set_done.wait_for(std::chrono::milliseconds{50}), std::future_status::timeout);

    release_callback.set_value();
    EXPECT_EQ(set_done.wait_for(std::chrono::seconds{5}), std::future_status::ready);
    invoker.join();

    sut.TryToEnvoke(0);
    EXPECT_EQ(replacement.Count(), 1U);
    EXPECT_EQ(replacement.Last(), 0);
}

}  // namespace
}  // namespace detail
}  // namespace time
}  // namespace score
