// Copyright 2018 The Cobalt Authors. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef STARBOARD_SHARED_UWP_LOG_WRITER_WIN32_H_
#define STARBOARD_SHARED_UWP_LOG_WRITER_WIN32_H_

#include <memory>

#include "starboard/shared/uwp/log_writer_interface.h"

namespace starboard {
namespace shared {
namespace uwp {

std::unique_ptr<ILogWriter> CreateLogWriterWin32(const char* path);

}  // namespace uwp
}  // namespace shared
}  // namespace starboard

#endif  // STARBOARD_SHARED_UWP_LOG_WRITER_WIN32_H_
