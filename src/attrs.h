#pragma once

#include <absl/hash/hash.h>
#include <SkColor.h>

#include <array>

namespace Colors {
  constexpr int kBlack = 0,
                kRed = 1,
                kGreen = 2,
                kYellow = 3,
                kBlue = 4,
                kMagenta = 5,
                kCyan = 6,
                kWhite = 7,

                // color + kBold == bolded color
                kBold = 8,

                kForeground = 16,
                kBackground = 17,

                kMax = 17;
}

using Theme = std::array<SkColor, Colors::kMax + 1>;

struct Attr {
  SkColor foreground, background;

  static constexpr int kBold = 1<<1,
                       kItalic = 1<<2,
                       kUnderline = 1<<3,
                       kInverse = 1<<4,
                       kProtect = 1<<5,
                       kDirty = 1<<6;
  int flags{0};

  bool operator==(const Attr &rhs) const {
    return foreground == rhs.foreground && background == rhs.background &&
           flags == rhs.flags;
  }

  template <typename H>
  friend H AbslHashValue(H h, const Attr& attr) {
    return H::combine(std::move(h), attr.foreground, attr.background, attr.flags);
  }
};
