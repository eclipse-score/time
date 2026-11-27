# *******************************************************************************
# Copyright (c) 2026 Contributors to the Eclipse Foundation
#
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
#
# This program and the accompanying materials are made available under the
# terms of the Apache License Version 2.0 which is available at
# https://www.apache.org/licenses/LICENSE-2.0
#
# SPDX-License-Identifier: Apache-2.0
# *******************************************************************************

"""Fixtures of the end-to-end tests of the components.

The component requirements are covered by unit tests as well, so these tests are skipped unless they are
enabled with --run-component-requirement-tests.
"""

import pytest
from fixtures import (  # noqa: F401  (fixtures, found by pytest in this module)
    add_host_clock_option,
    event_probe_stream,
    gptp_iface,
    grandmaster,
    host_clock_check,
    jump_detection_armed,
    platform,
    probe_stream,
    time_stack,
)


def pytest_addoption(parser):
    add_host_clock_option(parser)
    parser.addoption(
        "--run-component-requirement-tests",
        action="store_true",
        default=False,
        help="also run the tests of component requirements, which the unit tests cover as well",
    )


def pytest_collection_modifyitems(config, items):
    if config.getoption("--run-component-requirement-tests"):
        return
    skip = pytest.mark.skip(
        reason="component requirement test, enable with --run-component-requirement-tests"
    )
    for item in items:
        item.add_marker(skip)
