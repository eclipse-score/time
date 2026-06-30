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
#include "score/time/high_res_steady_time/src/high_res_steady_clock.h"

#include <benchmark/benchmark.h>

namespace score
{
namespace time
{

class HighResSteadyClockBenchmarkFixture : public benchmark::Fixture
{
  public:
    HighResSteadyClockBenchmarkFixture() : clock_{HighResSteadyClock::GetInstance()}
    {
        this->Repetitions(1);
        this->ReportAggregatesOnly(true);
        this->ThreadRange(1, 16);
        this->UseRealTime();
        this->MeasureProcessCPUTime();
    }

    HighResSteadyClock clock_;
};

BENCHMARK_F(HighResSteadyClockBenchmarkFixture, NowLatency)(benchmark::State& state)
{
    for (auto _ : state)
    {
        benchmark::DoNotOptimize(clock_.Now());
    }
}

BENCHMARK_MAIN();

}  // namespace time
}  // namespace score
