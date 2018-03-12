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

static constexpr Theme kDefaultTheme{
  SkColorSetRGB(0x00, 0x00, 0x00),  // kBlack
  SkColorSetRGB(0xcd, 0x00, 0x00),  // kRed
  SkColorSetRGB(0x00, 0xcd, 0x00),  // kGreen
  SkColorSetRGB(0xcd, 0xcd, 0x00),  // kYellow
  SkColorSetRGB(0x00, 0x00, 0xee),  // kBlue
  SkColorSetRGB(0xcd, 0x00, 0xcd),  // kMagenta
  SkColorSetRGB(0x00, 0xcd, 0xcd),  // kCyan
  SkColorSetRGB(0xe5, 0xe5, 0xe5),  // kWhite

  SkColorSetRGB(0x7f, 0x7f, 0x7f),  // kBlack + kBold
  SkColorSetRGB(0xff, 0x00, 0x00),  // kRed + kBold
  SkColorSetRGB(0x00, 0xff, 0x00),  // kGreen + kBold
  SkColorSetRGB(0xff, 0xff, 0x00),  // kYellow + kBold
  SkColorSetRGB(0x5c, 0x5c, 0xff),  // kBlue + kBold
  SkColorSetRGB(0xff, 0x00, 0xff),  // kMagenta + kBold
  SkColorSetRGB(0x00, 0xff, 0xff),  // kCyan + kBold
  SkColorSetRGB(0xff, 0xff, 0xff),  // kWhite + kBold

  SkColorSetRGB(0xff, 0xff, 0xff),         // kForeground
  SkColorSetARGB(0xef, 0x2a, 0x34, 0x39),  // kBackground
};

struct Attr {
  SkColor foreground, background;

  static constexpr int kBold = 1<<1,
                       kItalic = 1<<2,
                       kUnderline = 1<<3,
                       kInverse = 1<<4,
                       kProtect = 1<<5;
  int flags{0};
  bool dirty{false};

  bool operator==(const Attr &rhs) const {
    return foreground == rhs.foreground && background == rhs.background &&
           flags == rhs.flags && dirty == rhs.dirty;
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
      combine(&hash, attr.dirty);
      return hash;
    }
  };
};
