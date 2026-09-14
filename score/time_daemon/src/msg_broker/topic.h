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
#ifndef SCORE_TIME_DAEMON_SRC_MSG_BROKER_TOPIC_H
#define SCORE_TIME_DAEMON_SRC_MSG_BROKER_TOPIC_H

#include <functional>
#include <ostream>
#include <string>

namespace score::td
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
    auto operator=(const Topic&) noexcept -> Topic& = delete;
    Topic(Topic&&) noexcept = default;
    auto operator=(Topic&&) noexcept -> Topic& = delete;
    ~Topic() noexcept = default;

    [[nodiscard]] auto Name() const noexcept -> const std::string&;

  private:
    std::string name_;
    static constexpr std::size_t kMaxLength{32U};
};

auto operator==(const Topic& lhs, const Topic& rhs) noexcept -> bool;
auto operator!=(const Topic& lhs, const Topic& rhs) noexcept -> bool;
auto operator<(const Topic& lhs, const Topic& rhs) noexcept -> bool;

}  // namespace score::td

// Specialize hash for score::td::Topic
namespace std
{
template <>
struct hash<score::td::Topic>
{
    auto operator()(const score::td::Topic& topic) const noexcept -> std::size_t
    {
        return std::hash<std::string>()(topic.Name());
    }
};
}  // namespace std

#endif  // SCORE_TIME_DAEMON_SRC_MSG_BROKER_TOPIC_H
