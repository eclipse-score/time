/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_DAEMON_ARGPARSER_H_
#define SCORE_TIME_DAEMON_ARGPARSER_H_

#include <iterator>
#include <string_view>
#include <vector>

namespace score {
namespace time {
namespace daemon {

class ArgParser final {
public:
    ArgParser(int arg_count, const char* const arg_values []) noexcept;
    ~ArgParser() = default;

    bool IsDebugEnabled() const noexcept;
    bool IsHelpRequested() const noexcept;
    bool IsVersionRequested() const noexcept;

    static void PrintHelp() noexcept;
    static void PrintVersion() noexcept;

private:
    using container_type = std::vector<std::string_view>;
    using distance_type = std::iterator_traits<container_type::iterator>::difference_type;

    distance_type FindOption(std::string_view option) const noexcept;
    std::vector<std::string_view> args_;
    static constexpr int npos_ = -1;
};

}  // namespace daemon
}  // namespace time
}  // namespace score

#endif  // SCORE_TIME_DAEMON_ARGPARSER_H_
