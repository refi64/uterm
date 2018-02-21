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
  int font_defaults_size() const { return m_font_defaults_size; }
  const std::vector<Font> & fonts() const { return m_fonts; }
  const Theme & theme() const { return m_theme; }
private:
  string m_shell;

  static constexpr int kDefaultFontSize = 16;
  int m_font_defaults_size{kDefaultFontSize};
  std::vector<Font> m_fonts;

  Theme m_theme{kDefaultTheme};
};
