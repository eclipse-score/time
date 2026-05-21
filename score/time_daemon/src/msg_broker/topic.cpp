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
#include "score/time_daemon/src/msg_broker/topic.h"

#include "score/mw/log/logging.h"

namespace score
{
namespace td
{

Topic::Topic(const std::string& name) noexcept
{
    if (name.size() > kMaxLength)
    {
        score::mw::log::LogWarn() << name << " is to long, reducing to: " << kMaxLength << " max length";
        name_ = name.substr(0U, kMaxLength);
    }
    else
    {
        name_ = name;
    }
}

Topic::Topic(const char* name) noexcept : Topic(std::string(name)) {}

const std::string& Topic::Name() const noexcept
{
    return name_;
}

bool operator==(const Topic& lhs, const Topic& rhs) noexcept
{
    return lhs.Name() == rhs.Name();
}

bool operator!=(const Topic& lhs, const Topic& rhs) noexcept
{
    return !(lhs == rhs);
}

bool operator<(const Topic& lhs, const Topic& rhs) noexcept
{
    return lhs.Name() < rhs.Name();
}

}  // namespace td
}  // namespace score
