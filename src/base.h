#pragma once

#include <fmt/format.h>
#include <stdint.h>
#include <string>

#ifdef USE_LIBTSM_XKBCOMMON
#include "../deps/libtsm/external/xkbcommon-keysyms.h"
#else
#include <xkbcommon/xkbcommon-keysyms.h>
#endif

using std::string;
using std::u32string;
using uint = unsigned int;
using uint32 = uint32_t;
using uint64 = uint64_t;

// The state of an active selections; it's either just begun, is being
// updated, or just finished.
enum class Selection { kBegin, kUpdate, kEnd };

// The scrolling direction.
enum class ScrollDirection { kUp, kDown };
