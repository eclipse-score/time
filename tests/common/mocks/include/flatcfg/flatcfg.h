/********************************************************************************
 * (c) 2025 ETAS GmbH. All rights reserved.
 ********************************************************************************/

#ifndef FLATCFG_H_
#define FLATCFG_H_

#include <gmock/gmock.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "flatcfg/flatcfg_error.h"
#include "score/result/result.h"

namespace flatcfg {
class Mock_Flatcfg {
public:
    MOCK_METHOD0(getSwClusterList, score::Result<std::vector<std::string>>());
    MOCK_METHOD1(load, score::Result<std::shared_ptr<void>>(const std::string&));
};

class FlatCfg {
public:
    static Mock_Flatcfg* mock_flat_cfg_;

    template <typename T>
    FlatCfg(const T&) {
    }
    score::Result<std::vector<std::string>> getSwClusterList() {
        return mock_flat_cfg_->getSwClusterList();
    }
    score::Result<std::shared_ptr<void>> load(const std::string& swcl) {
        return mock_flat_cfg_->load(swcl);
    }
};

}  // namespace flatcfg
#endif  // FLATCFG_H_
