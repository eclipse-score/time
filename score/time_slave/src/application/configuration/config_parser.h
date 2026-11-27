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
#ifndef SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_PARSER_H
#define SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_PARSER_H

#include "score/time_slave/src/application/configuration/time_slave_config.h"

#include <string>
#include <vector>

namespace score
{
namespace ts
{

/// Outcome of ParseConfig(): either a fully valid config, or the list of problems found.
struct ConfigParseResult
{
    /// Valid only if ok(). On errors it may be partially filled and must not be used.
    TimeSlaveConfig config{};

    /// One human-readable message per problem (all problems are collected, not just the first).
    std::vector<std::string> errors{};

    bool ok() const
    {
        return errors.empty();
    }
};

/**
 * @brief Parse and validate a TimeSlave JSON configuration file from @p path.
 *
 * Missing optional fields keep the defaults described in the JSON schema.
 *
 * Every constraint of the schema is enforced: value types (a JSON boolean is not an
 * integer and vice versa), numeric ranges (``minimum`` / ``maximum``) and unknown keys
 * (``additionalProperties: false``). Invalid values are never clamped or applied; they are
 * reported in ConfigParseResult::errors. Invalid JSON or a root that is not an object is
 * reported the same way. A path that is not a regular file (a directory or a missing file)
 * is not a config file: the built-in defaults are returned with no errors.
 *
 * @param path  Filesystem path to the JSON config file.
 * @return Parsed config, or the list of errors (check ConfigParseResult::ok()).
 */
ConfigParseResult ParseConfig(const std::string& path);

}  // namespace ts
}  // namespace score

#endif  // SCORE_TIME_SLAVE_CONFIGURATION_CONFIG_PARSER_H
