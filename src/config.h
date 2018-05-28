#pragma once

#include "base.h"
#include "error.h"
#include "attrs.h"

#include <confuse.h>

#include <vector>

class Config {
public:
  struct Font {
    string name;
    int size;
  };

  Config();
  Error Parse();

  const string & shell() const { return m_shell; }
  int vsync() const { return m_vsync; }
  int fps() const { return m_fps; }
  int font_defaults_size() const { return m_font_defaults_size; }
  const std::vector<Font> & fonts() const { return m_fonts; }
  const Theme & theme() const { return m_theme; }
private:
  string m_shell;
  int m_vsync, m_fps;

  static constexpr int kDefaultFontSize = 16;
  int m_font_defaults_size{kDefaultFontSize};
  std::vector<Font> m_fonts;

  // Work around a bug in GCC <4.9: https://stackoverflow.com/q/32912921/
  Theme m_theme = kDefaultTheme;
};
