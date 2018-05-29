#pragma once

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

  struct Hash {
    template <typename T>
    static void combine(size_t *current, T value) {
      std::hash<T> hasher;
      *current ^= hasher(value) + 0x9e3779b9 + (*current << 6) + (*current >> 2);
    }

    size_t operator()(const Attr &attr) const {
      size_t hash = 0;
      combine(&hash, attr.foreground);
      combine(&hash, attr.background);
      combine(&hash, attr.flags);
      return hash;
    }
  };
};
