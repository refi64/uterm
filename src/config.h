#pragma once

#include "base.h"
#include "error.h"
#include "attrs.h"

#include <vector>


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
  SkColorSetRGB(0xcd, 0x00, 0x00),  // kRed + kBold
  SkColorSetRGB(0x00, 0xcd, 0x00),  // kGreen + kBold
  SkColorSetRGB(0xcd, 0xcd, 0x00),  // kYellow + kBold
  SkColorSetRGB(0x00, 0x00, 0xee),  // kBlue + kBold
  SkColorSetRGB(0xcd, 0x00, 0xcd),  // kMagenta + kBold
  SkColorSetRGB(0x00, 0xcd, 0xcd),  // kCyan + kBold
  SkColorSetRGB(0xff, 0xff, 0xff),  // kWhite + kBold

  SkColorSetRGB(0xff, 0xff, 0xff),         // kForeground
  SkColorSetARGB(0xef, 0x2a, 0x34, 0x39),  // kBackground
};

class Config {
public:
  struct Font {
    string name;
    int size;
  };

  Config();
  Error Parse();

  const string & shell() const { return m_shell; }
  bool hwaccel() const { return m_hwaccel; }
  int vsync() const { return m_vsync; }
  int fps() const { return m_fps; }
  int font_defaults_size() const { return m_font_defaults_size; }
  const std::vector<Font> & fonts() const { return m_fonts; }
  const Theme & theme() const { return m_theme; }
private:
  string m_shell;
  bool m_hwaccel;
  int m_vsync, m_fps;

  static constexpr int kDefaultFontSize = 16;
  int m_font_defaults_size{kDefaultFontSize};
  std::vector<Font> m_fonts;

  // Work around a bug in GCC <4.9: https://stackoverflow.com/q/32912921/
  Theme m_theme = kDefaultTheme;
};
