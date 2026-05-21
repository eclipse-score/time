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
#ifndef SCORE_TIMEDAEMON_CODE_MSG_BROKER_TOPIC_H
#define SCORE_TIMEDAEMON_CODE_MSG_BROKER_TOPIC_H

#include <functional>
#include <ostream>
#include <string>

namespace score
{
namespace td
{

///
/// \brief Class to store the name for each topic that will be necessary for message broker
///
class Topic
{
  public:
    explicit Topic(const std::string& name) noexcept;
    Topic(const char* name) noexcept;

    Topic(const Topic&) = default;
    Topic& operator=(const Topic&) noexcept = delete;
    Topic(Topic&&) noexcept = default;
    Topic& operator=(Topic&&) noexcept = delete;
    ~Topic() noexcept = default;

    const std::string& Name() const noexcept;

  private:
    std::string name_;
    const std::size_t kMaxLength{32U};
};

bool operator==(const Topic& lhs, const Topic& rhs) noexcept;
bool operator!=(const Topic& lhs, const Topic& rhs) noexcept;
bool operator<(const Topic& lhs, const Topic& rhs) noexcept;

}  // namespace td
}  // namespace score

// Specialize hash for score::td::Topic
namespace std
{
template <>
struct hash<score::td::Topic>
{
  std::size_t operator()(const score::td::Topic& t) const noexcept
    {
        return std::hash<std::string>()(t.Name());
    }
};
}  // namespace std

#endif  // SCORE_TIMEDAEMON_CODE_MSG_BROKER_TOPIC_H
