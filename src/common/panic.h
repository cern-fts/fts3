/* Copyright @ CERN, 2014.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License. */

/** \file panic.h Set up logic to handle expected/unexpected signals,
 *                trying to orderly shut down, and die after giving a chance */

#pragma once

#include "common_dev.h"

#include <string>

FTS3_COMMON_NAMESPACE_START

namespace Panic
{

#define STACK_BACKTRACE_SIZE 25

extern void* stack_backtrace[];
extern int stack_backtrace_size;

void setup_signal_handlers(void (*shutdown_callback)(int, void*), void* udata);

std::string stack_dump(void *stack[], int size);

}

FTS3_COMMON_NAMESPACE_END
