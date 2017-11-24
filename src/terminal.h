#pragma once

#include "uterm.h"
#include "error.h"
#include "pty.h"
#include "attrs.h"

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

  bool operator==(Pos rhs) { return x == rhs.x && y == rhs.y; }
};

struct SelectionRange { Pos begin{0, 0}, end{0, 0}, origin{0, 0}; };

class Terminal {
public:
  Terminal();

  using DrawCb = std::function<void(const u32string&, Pos, Attr, int)>;
  using CopyCb = std::function<void(const string&)>;
  using PasteCb = std::function<string()>;

  void set_draw_cb(DrawCb draw_cb);
  void set_copy_cb(CopyCb copy_cb);
  void set_paste_cb(PasteCb paste_cb);
  void set_pty(Pty* pty);

  Pos cursor();

  void SetSelection(Selection state, uint x, uint y);
  void EndSelection();
  void ResetSelection();

  void Scroll(ScrollDirection direction, uint distance);

  const Attr & default_attr() { return m_default_attr; }
  Error Resize(int x, int y);
  void WriteToScreen(string text);
  bool WriteKeysymToPty(uint32 keysym, int mods);
  bool WriteUnicodeToPty(uint32 code);
  void Draw();
private:
  static void StaticWrite(tsm_vte *vte, const char *u8, size_t len, void *data);
  static int StaticDraw(tsm_screen *screen, uint32 id, const uint32 *chars, size_t len,
                        uint width, uint posx, uint posy, const tsm_screen_attr *tattr,
                        tsm_age_t age, void *data);

  DrawCb m_draw_cb;
  CopyCb m_copy_cb;
  PasteCb m_paste_cb;

  tsm_screen *m_screen;
  tsm_vte *m_vte;

  SelectionRange m_selection_range;
  string m_selection_contents;
  int m_age{0};
  Attr m_default_attr;
  Pty *m_pty;
};
