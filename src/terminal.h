#pragma once

#include "uterm.h"
#include "error.h"
#include "pty.h"

#include <SkColor.h>
#include <functional>

#include <libtsm.h>

namespace KeyboardModifier {
  constexpr int kShift = TSM_SHIFT_MASK,
                kControl = TSM_CONTROL_MASK,
                kAlt = TSM_ALT_MASK,
                kSuper = TSM_LOGO_MASK;
};

struct Pos {
  Pos(uint nx, uint ny): x{nx}, y{ny} {}
  uint x, y;
};

struct Attr {
  SkColor foreground, background;

  static constexpr int kBold = 1<<1,
                       kUnderline = 1<<2,
                       kInverse = 1<<3,
                       kProtect = 1<<4;
  int flags;

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

class Terminal {
public:
  Terminal();

  using DrawCb = std::function<void(const u32string&, Pos, Attr, int)>;

  void set_draw_cb(DrawCb draw_cb);
  void set_pty(Pty* pty);

  Pos cursor();

  void Resize(int x, int y);
  void WriteToScreen(string text);
  bool WriteKeysymToPty(uint32 keysym, int mods);
  bool WriteUnicodeToPty(uint32 code);
  void Draw();
private:
  static void StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data);
  static int StaticDraw(tsm_screen *screen, uint32 id, const uint32 *chars, size_t len,
                        uint width, uint posx, uint posy, const tsm_screen_attr *attr,
                        tsm_age_t age, void *data);

  DrawCb m_draw_cb;

  tsm_screen *m_screen;
  tsm_vte *m_vte;

  int m_age{0};
  Pty *m_pty;
};
