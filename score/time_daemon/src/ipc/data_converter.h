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
#ifndef SCORE_TIMEDAEMON_CODE_IPC_DATA_CONVERTER_H
#define SCORE_TIMEDAEMON_CODE_IPC_DATA_CONVERTER_H

#include <type_traits>

namespace score
{
namespace td
{

template <typename>
struct DependentFalse : std::false_type
{
};

template <typename Src, typename Dst>
struct DataConverter
{
    static Dst Convert(const Src&)
    {
        static_assert(DependentFalse<Src>::value,
                      "Missing DataConverter<Src, Dst> specialization for this PublisherImpl message type.");
        return {};
    }
};

/**
 * \brief Helper function to convert data to ipc data using the DataConverter struct
 */
template <typename Dst, typename Src>
inline Dst ConvertToIpcData(const Src& src)
{
    return DataConverter<Src, Dst>::Convert(src);
}

}  // namespace td
}  // namespace score

#endif  // SCORE_TIMEDAEMON_CODE_IPC_DATA_CONVERTER_H
