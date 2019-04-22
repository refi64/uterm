#include "config.h"

#include <absl/strings/ascii.h>

#include <confuse.h>

#include <algorithm>
#include <iterator>
#include <unistd.h>

static bool FileExists(string path) {
  return access(path.c_str(), R_OK) != -1;
}

static string operator/(string a, const string& b) {
  int offs = 0;
  if (a.back() != '/' && b[0] != '/') {
    a.push_back('/');
  }
  if (a.back() == '/' && b[0] == '/') {
    offs = 1;
  }

  a.reserve(a.size() + b.size() - offs);
  std::copy(b.begin() + offs, b.end(), std::back_inserter(a));
  return a;
}

static string GetShell() {
  const char *shell = getenv("SHELL");
  return shell ? shell : "/bin/sh";
}

static Expect<string> GetHome() {
  const char *home = getenv("HOME");
  if (home == nullptr) {
    return Expect<string>::WithError("$HOME is not defined.");
  } else {
    return Expect<string>::New(string{home});
  }
}

static Expect<string> GetConfigDir() {
  #ifdef __linux__
  string dir;

  const char *env = getenv("XDG_CONFIG_HOME");
  if (env != nullptr) {
    dir = env;
  } else {
    auto home = GetHome();
    if (auto err = home.Error()) {
      return err.Extend("$XDG_CONFIG_HOME is not defined, either.");
    }
    dir = *home / ".config";
  }

  return Expect<string>::New(dir / "uterm");
  #else
  auto home = GetHome();
  if (auto err = home.Error()) {
    return err;
  }

  return Expect<string>::New(*home / ".uterm");
  #endif
}

static Expect<string> GetConfigPath() {
  auto dir = GetConfigDir();
  if (auto err = dir.Error()) {
    return err;
  }

  return Expect<string>::New(*dir / "uterm.conf");
}

int VerifyColorCb(cfg_t *cfg, cfg_opt_t *opt, const char *c_value, void *v_result) {
  static_assert(std::numeric_limits<long>::max() >= 0xFFFFFFFF,
                "long should be 64 bits.");

  long *result = static_cast<long*>(v_result);
  string value{c_value};

  if ((value.size() != 8 && value.size() != 10) ||
      !(value[0] == '0' && value[1] == 'x') ||
      !std::all_of(value.begin() + 2, value.end(), absl::ascii_isxdigit)) {
    cfg_error(cfg, "Invalid hex color value in '%s': %s", opt->name, c_value);
    return -1;
  }

  long color = std::stol(value, nullptr, 16);
  if (value.size() == 8) {
    // No alpha value.
    *result = 0xFF000000 | color;
  } else {
    // Skia stores the alpha value at top, but traditional RGBA stores it at the bottom.
    *result = (color << 24) | (color >> 8);
  }
  return 0;
}

Config::Config() {
  m_shell = GetShell();
}

Error Config::Parse() {
  auto path = GetConfigPath();
  if (auto err = path.Error()) {
    return err;
  } else if (!FileExists(*path)) {
    return Error::New();
  }

  cfg_opt_t theme_opts[] = {
    #define CFG_COLOR(name, def) \
      CFG_INT_CB(#name, def, CFGF_NONE, VerifyColorCb)

    CFG_COLOR(black, kDefaultTheme[Colors::kBlack]),
    CFG_COLOR(red, kDefaultTheme[Colors::kRed]),
    CFG_COLOR(green, kDefaultTheme[Colors::kGreen]),
    CFG_COLOR(yellow, kDefaultTheme[Colors::kYellow]),
    CFG_COLOR(blue, kDefaultTheme[Colors::kBlue]),
    CFG_COLOR(magenta, kDefaultTheme[Colors::kMagenta]),
    CFG_COLOR(cyan, kDefaultTheme[Colors::kCyan]),
    CFG_COLOR(white, kDefaultTheme[Colors::kWhite]),

    CFG_COLOR(bright_black, kDefaultTheme[Colors::kBlack + Colors::kBold]),
    CFG_COLOR(bright_red, kDefaultTheme[Colors::kRed + Colors::kBold]),
    CFG_COLOR(bright_green, kDefaultTheme[Colors::kGreen+ Colors::kBold]),
    CFG_COLOR(bright_yellow, kDefaultTheme[Colors::kYellow + Colors::kBold]),
    CFG_COLOR(bright_blue, kDefaultTheme[Colors::kBlue + Colors::kBold]),
    CFG_COLOR(bright_magenta, kDefaultTheme[Colors::kMagenta + Colors::kBold]),
    CFG_COLOR(bright_cyan, kDefaultTheme[Colors::kCyan + Colors::kBold]),
    CFG_COLOR(bright_white, kDefaultTheme[Colors::kWhite + Colors::kBold]),

    CFG_COLOR(foreground, kDefaultTheme[Colors::kForeground]),
    CFG_COLOR(background, kDefaultTheme[Colors::kBackground]),

    #undef CFG_COLOR

    CFG_END()
  };

  cfg_opt_t font_opts[] = {
    CFG_INT("size", 0, CFGF_NONE),
    CFG_END()
  };

  cfg_opt_t font_defaults_opts[] = {
    CFG_INT("size", 0, CFGF_NONE),
    CFG_END()
  };

  cfg_opt_t opts[] = {
    // Note that the const_cast is only needed on libconfuse versions <v2.8.
    CFG_STR("shell", const_cast<char*>(m_shell.c_str()), CFGF_NONE),
    CFG_BOOL("hwaccel", cfg_true, CFGF_NONE),
    CFG_INT("vsync", -1, CFGF_NONE),
    CFG_INT("fps", 120, CFGF_NONE),

    CFG_SEC("theme", theme_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NO_TITLE_DUPES),
    CFG_STR("current-theme", "", CFGF_NONE),

    CFG_SEC("font", font_opts, CFGF_MULTI | CFGF_TITLE | CFGF_NO_TITLE_DUPES),
    CFG_SEC("font-defaults", font_defaults_opts, CFGF_NONE),

    CFG_END()
  };

  cfg_t *cfg = cfg_init(opts, CFGF_NOCASE);

  int ret = cfg_parse(cfg, path->c_str());
  if (ret == CFG_FILE_ERROR) {
    return Error::Errno().Extend("while reading config file");
  } else if (ret == CFG_PARSE_ERROR) {
    return Error::New();
  }

  m_shell = cfg_getstr(cfg, "shell");
  m_hwaccel = cfg_getbool(cfg, "hwaccel");
  m_vsync = cfg_getint(cfg, "vsync");
  m_fps = cfg_getint(cfg, "fps");

  const char *wanted_theme = cfg_getstr(cfg, "current-theme");
  int themes = cfg_size(cfg, "theme");
  for (int i = 0; i < themes; i++) {
    cfg_t *cfg_theme = cfg_getnsec(cfg, "theme", i);
    if (strcmp(cfg_title(cfg_theme), wanted_theme) != 0) {
      continue;
    }

    for (int c = 0; c <= Colors::kMax; c++) {
      m_theme[c] = cfg_getint(cfg_theme, theme_opts[c].name);
    }
  }

  m_font_defaults_size = cfg_getint(cfg_getsec(cfg, "font-defaults"), "size");
  if (m_font_defaults_size == 0) {
    m_font_defaults_size = kDefaultFontSize;
  }

  int fonts = cfg_size(cfg, "font");
  for (int i = 0; i < fonts; i++) {
    cfg_t *cfg_font = cfg_getnsec(cfg, "font", i);

    int size = cfg_getint(cfg_font, "size");
    if (size == 0) {
      size = m_font_defaults_size;
    }

    m_fonts.push_back(Font{cfg_title(cfg_font), size});
  }

  return Error::New();
}
