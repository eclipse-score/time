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
#include "score/time_slave/src/application/time_slave.h"

#include "src/lifecycle_client_lib/include/runapplication.h"

int main(int argc, const char* argv[])
{
    return score::mw::lifecycle::run_application<score::ts::TimeSlave>(argc, argv);
}
