#pragma once

#include "base.h"
#include "error.h"
#include "attrs.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <GrBackendSurface.h>
#include <GrContext.h>
#include <SkCanvas.h>
#include <SkSurface.h>

#include <functional>

class Window {
public:
  using KeyCb = std::function<void(uint32, int)>;
  using CharCb = std::function<void(uint)>;
  using ResizeCb = std::function<void(int, int)>;
  using SelectionCb = std::function<void(Selection, double, double)>;
  using ScrollCb = std::function<void(ScrollDirection, uint)>;

  Window();
  ~Window();

  void set_key_cb(KeyCb key_cb);
  void set_char_cb(CharCb char_cb);
  void set_resize_cb(ResizeCb resize_cb);
  void set_selection_cb(SelectionCb selection_cb);
  void set_scroll_cb(ScrollCb scroll_cb);

  Error Initialize(int width, int height, const Theme& theme);
  bool isopen();
  SkCanvas * canvas() { return m_surface->getCanvas(); }

  string ClipboardRead();
  void ClipboardWrite(const string &str);
  void SetTitle(const string &str);

  void DrawAndPoll(bool significant_redraw);
private:
  const Theme *m_theme{nullptr};

  KeyCb m_key_cb;
  CharCb m_char_cb;
  ResizeCb m_resize_cb;
  SelectionCb m_selection_cb;
  ScrollCb m_scroll_cb;

  Error CreateSurface();

  static void StaticKeyCallback(GLFWwindow *glfw_window, int key, int scancode,
                                int action, int glfw_mods);
  static void StaticCharCallback(GLFWwindow *glfw_window, uint code);
  static void StaticWinResizeCallback(GLFWwindow *glfw_window, int width, int height);
  static void StaticFbResizeCallback(GLFWwindow *glfw_window, int width, int height);
  static void StaticMouseCallback(GLFWwindow *glfw_window, int button, int action,
                                  int mods);
  static void StaticScrollCallback(GLFWwindow *glfw_window, double xoffset,
                                   double yoffset);

  GLFWwindow *m_window{nullptr};
  GLFWcursor *m_cursor{nullptr};
  int m_fb_width, m_fb_height;
  bool m_selection_active{false};

  GrGLFramebufferInfo m_info;
  sk_sp<const GrGLInterface> m_interface;
  sk_sp<GrContext> m_context;
  std::unique_ptr<GrBackendRenderTarget> m_target;
  sk_sp<SkSurface> m_surface;
};
