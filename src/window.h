#pragma once

#include "uterm.h"
#include "error.h"

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

  Window();
  ~Window();

  void set_key_cb(KeyCb key_cb);
  void set_char_cb(CharCb char_cb);
  void set_resize_cb(ResizeCb resize_cb);

  Error Initialize(int width, int height);
  bool isopen();
  SkCanvas * canvas() { return m_surface->getCanvas(); }
  void Draw();
private:
  KeyCb m_key_cb;
  CharCb m_char_cb;
  ResizeCb m_resize_cb;

  Error CreateSurface();

  static void StaticKeyCallback(GLFWwindow *glfw_window, int key, int scancode,
                                int action, int glfw_mods);
  static void StaticCharCallback(GLFWwindow *glfw_window, uint code);
  static void StaticWinResizeCallback(GLFWwindow *glfw_window, int width, int height);
  static void StaticFbResizeCallback(GLFWwindow *glfw_window, int width, int height);

  GLFWwindow *m_window{nullptr};
  int m_fb_width, m_fb_height;
  int m_win_resize_width{-1}, m_win_resize_height{-1};
  GrGLFramebufferInfo m_info;
  sk_sp<const GrGLInterface> m_interface;
  sk_sp<GrContext> m_context;
  std::unique_ptr<GrBackendRenderTarget> m_target;
  sk_sp<SkSurface> m_surface;
};
