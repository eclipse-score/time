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
#include "score/time/high_res_steady_time/src/details/qtime/high_res_steady_qclock.h"
#include "score/time/high_res_steady_time/src/details/qtime/tick_provider_mock.h"

#include "score/lib/os/mocklib/qnx/neutrino_qnx_mock.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace score
{
namespace time
{
namespace high_res_steady_time
{
namespace qtime
{
namespace test
{

using namespace ::testing;

struct TestCaseParams
{
    std::uint64_t tickCount{};
    std::uint64_t tickPerSec{};
    std::uint64_t tickInNano{};

    friend void PrintTo(const TestCaseParams& p, std::ostream* os)
    {
        *os << "tickCount=" << p.tickCount << ", tickPerSec=" << p.tickPerSec
            << ", tickInNano=" << p.tickInNano;
    }
};

class HighResSteadyQClockTest : public ::testing::TestWithParam<TestCaseParams>
{
  public:
    void SetUp() override
    {
        TickProviderMock::CreateMockInstance();
    }

    void TearDown() override
    {
        TickProviderMock::DestroyMockInstance();
    }
};

// clang-format off
INSTANTIATE_TEST_SUITE_P(
    HighResSteadyQClockInstantiate,
    HighResSteadyQClockTest,
    ::testing::Values(
        // Zero ticks
        TestCaseParams{0,                       19'200'000,         0},
        // Some values
        TestCaseParams{3'462'637'232,            19'200'000,         180'345'689'166},
        // ~960 s
        TestCaseParams{2'266'560'000'000,        2'361'000'000,      960'000'000'000},
        // ~1 hour
        TestCaseParams{8'499'600'654'123,        2'361'000'000,      3'600'000'277'053},
        // ~24 hours
        TestCaseParams{203'990'415'698'952,      2'361'000'000,      86'400'006'649'280},
        // ~48 hours
        TestCaseParams{407'980'831'397'904,      2'361'000'000,      172'800'013'298'561},
        // ~96 hours
        TestCaseParams{815'961'662'795'808,      2'361'000'000,      345'600'026'597'123},
        // ~22 days
        TestCaseParams{0x000FFFFFFFFFFFFF,       2'361'000'000,      1'907'496'665'552'941},
        // ~353 days
        TestCaseParams{0x00FFFFFFFFFFFFFF,       2'361'000'000,      30'519'946'648'847'071},
        // ~15 years
        TestCaseParams{0x0FFFFFFFFFFFFFFF,       2'361'000'000,      488'319'146'381'553'144},
        // ~247 years
        TestCaseParams{0xFFFFFFFFFFFFFFFF,       2'361'000'000,      7'813'106'342'104'850'324},
        // boundary: cycles_per_sec == max
        TestCaseParams{0xFFFFFFFFFFFFFFFF,       0xFFFFFFFFFFFFFFFF, 1000000000},
        // overflow → returns 0
        TestCaseParams{0xFFFFFFFFFFFFFFFF,       1,                  0},
        TestCaseParams{0xFFFFFFFFFFFFFFFF,       1'000'000'000,      0},
        TestCaseParams{0xFFFFFFFFF,              1,                  0},
        // zero cycles_per_sec → returns 0
        TestCaseParams{3'462'637'232,            0,                  0}
    ));
// clang-format on

TEST_P(HighResSteadyQClockTest, ConvertsCyclesToNanoseconds)
{
    RecordProperty("Description",
                   "Verifies that HighResSteadyQClock::Now() converts QNX hardware clock cycles to nanoseconds "
                   "using the equation: ns = cycles * 1e9 / cycles_per_sec");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("DerivationTechnique", "Analysis of requirements");
    RecordProperty("ASIL", "QM");

    const auto params = GetParam();

    score::os::qnx::NeutrinoMock neutrino_mock;
    score::os::qnx::Neutrino::set_testing_instance(neutrino_mock);

    EXPECT_CALL(neutrino_mock, ClockCycles()).Times(Exactly(1)).WillOnce(Return(params.tickCount));
    EXPECT_CALL(TickProviderMock::GetMockInstance(), GetClockCyclesPerSec())
        .Times(Exactly(1))
        .WillOnce(Return(params.tickPerSec));

    HighResSteadyQClock clock;
    const auto snapshot = clock.Now();

    EXPECT_EQ(params.tickInNano, snapshot.TimePointNs().count());
}

TEST(HighResSteadyQClockTest, WhenClockCyclesReturnZeroNowReturnsZero)
{
    RecordProperty("Description",
                   "Fault injection: Neutrino::ClockCycles() returns 0 — Now() must return 0 ns.");
    RecordProperty("TestType", "Fault injection test");
    RecordProperty("ASIL", "QM");

    constexpr std::uint64_t kZeroCycles    = 0U;
    constexpr std::uint64_t kValidCpsValue = 10U;

    TickProviderMock::CreateMockInstance();
    auto& tick_mock = TickProviderMock::GetMockInstance();

    score::os::qnx::NeutrinoMock neutrino_mock;
    score::os::qnx::Neutrino::set_testing_instance(neutrino_mock);

    EXPECT_CALL(neutrino_mock, ClockCycles()).WillOnce(Return(kZeroCycles));
    EXPECT_CALL(tick_mock, GetClockCyclesPerSec()).WillOnce(Return(kValidCpsValue));

    HighResSteadyQClock clock;
    EXPECT_EQ(clock.Now().TimePointNs().count(), 0);

    TickProviderMock::DestroyMockInstance();
}

TEST(HighResSteadyQClockTest, WhenClockCyclesAreValidNowReturnsCorrectNanoseconds)
{
    RecordProperty("Description",
                   "Verifies that HighResSteadyQClock::Now() returns the correct ns for known valid inputs.");
    RecordProperty("ASIL", "QM");

    constexpr std::uint64_t kCps    = 100U;
    constexpr std::uint64_t kCycles = 25U;

    TickProviderMock::CreateMockInstance();
    auto& tick_mock = TickProviderMock::GetMockInstance();

    score::os::qnx::NeutrinoMock neutrino_mock;
    score::os::qnx::Neutrino::set_testing_instance(neutrino_mock);

    EXPECT_CALL(tick_mock, GetClockCyclesPerSec()).WillOnce(Return(kCps));
    EXPECT_CALL(neutrino_mock, ClockCycles()).WillOnce(Return(kCycles));

    HighResSteadyQClock clock;
    EXPECT_EQ(clock.Now().TimePointNs().count(),
              kCycles * 1'000'000'000U / kCps);

    TickProviderMock::DestroyMockInstance();
}

}  // namespace test
}  // namespace qtime
}  // namespace high_res_steady_time
}  // namespace time
}  // namespace score
