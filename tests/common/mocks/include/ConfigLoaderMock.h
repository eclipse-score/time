/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef SCORE_TIME_CONFIG_LOADER_MOCK_H_
#define SCORE_TIME_CONFIG_LOADER_MOCK_H_

#include <gmock/gmock.h>

#include <memory>

#include "ConfigLoader.h"

namespace score {
namespace time {
class TsyncConfigLoaderMock {
public:
    MOCK_CONST_METHOD0(GetConfig, const TimeBaseConfiguration&());
    MOCK_METHOD0(DumpConfig, void());
};

extern std::unique_ptr<TsyncConfigLoaderMock> config_loader_mock;

ConfigLoader::ConfigLoader() {
}

const TimeBaseConfiguration& ConfigLoader::GetConfig() const noexcept {
    return config_loader_mock->GetConfig();
}

void ConfigLoader::DumpConfig() noexcept {
    config_loader_mock->DumpConfig();
}

} /* namespace time */
} /* namespace score */

#endif
