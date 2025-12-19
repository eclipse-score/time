/*
 * Copyright 2017 Google Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef FLATBUFFERS_STL_EMULATION_H_
#define FLATBUFFERS_STL_EMULATION_H_

#include <limits>
#include <memory>
#include <type_traits>

namespace flatbuffers {

template <typename T>
using numeric_limits = std::numeric_limits<T>;
template <typename T> using is_scalar = std::is_scalar<T>;
template <typename T, typename U> using is_same = std::is_same<T,U>;
template <typename T> using is_floating_point = std::is_floating_point<T>;
template <typename T> using is_unsigned = std::is_unsigned<T>;
template <typename T> using is_enum = std::is_enum<T>;
template <typename T> using make_unsigned = std::make_unsigned<T>;
template<bool B, class T, class F>
using conditional = std::conditional<B, T, F>;
template<class T, T v>
using integral_constant = std::integral_constant<T, v>;
template <class T> using unique_ptr = std::unique_ptr<T>;

}  // namespace flatbuffers

#endif  // FLATBUFFERS_STL_EMULATION_H_
