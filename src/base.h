#pragma once

#include <fmt/format.h>
#include <stdint.h>
#include <string>

using std::string;
using std::u32string;
using uint = unsigned int;
using uint32 = uint32_t;

// The state of an active selections; it's either just begun, is being
// updated, or just finished.
enum class Selection { kBegin, kUpdate, kEnd };

// The scrolling direction.
enum class ScrollDirection { kUp, kDown };
