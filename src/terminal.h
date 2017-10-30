#pragma once

#include "uterm.h"
#include "error.h"
#include "pty.h"

#include <functional>

#include <libtsm.h>

namespace KeyboardModifier {
  constexpr int kShift = TSM_SHIFT_MASK,
                kLock = TSM_LOCK_MASK,
                kControl = TSM_CONTROL_MASK,
                kAlt = TSM_ALT_MASK,
                kLogo = TSM_LOGO_MASK;
};

struct Pos { int x, y; };

class Terminal {
public:
  Terminal();

  using DrawCb = std::function<void(const string&, Pos)>;

  void set_draw(DrawCb draw);
  void set_pty(Pty* pty);
  void WriteToScreen(string text);
  Error KeyboardToFd(uint32 keysym, int mods);
  void Draw();
private:
  static void StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data);
  static int StaticDraw(tsm_screen *screen, uint32 id, const uint32 *chars, size_t len,
                        uint width, uint posx, uint posy, const tsm_screen_attr *attr,
                        tsm_age_t age, void *data);

  DrawCb m_draw;

  tsm_screen *m_screen;
  tsm_vte *m_vte;

  int m_age{0};
  Pty *m_pty;
};
