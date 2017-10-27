#pragma once

#include "uterm.h"
#include "error.h"

#include <GL/gl3w.h>
#include <GLFW/glfw3.h>

#include <GrBackendSurface.h>
#include <GrContext.h>
#include <SkCanvas.h>
#include <SkSurface.h>

#include <memory>

class Window {
public:
  Window();
  ~Window();

  Error&& Initialize(int width, int height);
  bool isopen();
  SkCanvas* canvas() { return m_surface->getCanvas(); }
  void Draw();
private:
  GLFWwindow* m_window{nullptr};
  int m_width, m_height;
  sk_sp<const GrGLInterface> m_interface;
  sk_sp<GrContext> m_context;
  std::unique_ptr<GrBackendRenderTarget> m_target;
  sk_sp<SkSurface> m_surface;
};
