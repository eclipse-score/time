/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#include "score/time/daemon/ArgParser.h"

#include <algorithm>
#include <iostream>

namespace score {
namespace time {
namespace daemon {

// coverity[exn_spec_violation] There will never be enough arguments for a length_error to be thrown
ArgParser::ArgParser(int arg_count, const char* const arg_values[]) noexcept {
    for (int i = 0; i < arg_count; ++i) {
        args_.push_back({arg_values[i]});
    }
}

bool ArgParser::IsDebugEnabled() const noexcept {
    return (FindOption("-d") != ArgParser::npos_);
}

bool ArgParser::IsHelpRequested() const noexcept {
    return (FindOption("-h") != ArgParser::npos_);
}

bool ArgParser::IsVersionRequested() const noexcept {
    return (FindOption("-v") != ArgParser::npos_) || (FindOption("--version") != ArgParser::npos_);
}

ArgParser::distance_type ArgParser::FindOption(std::string_view option) const noexcept {
    for (auto it = args_.begin(); it != args_.end(); ++it) {
        if (*it == option) {
            return std::distance(args_.begin(), it);
        }
    }

    return -1;
}

void ArgParser::PrintHelp() noexcept {
    static const char* help =
        "timesync daemon options:\n"
        "----------------------------------------------------------------------------------------------------\n\n"
        "-d                     : [optional] debug mode, will yield more output from the daemon.\n"
        "-h                     : print this help screen.\n\n"
        "-v, --version          : print the version of the daemon.\n\n"
        "----------------------------------------------------------------------------------------------------\n";

    std::cout << help;
}

void ArgParser::PrintVersion() noexcept {
    std::cout << "Timesync Daemon Version: 0.0.0" << std::endl;
}

}  // namespace daemon
}  // namespace time
}  // namespace score
