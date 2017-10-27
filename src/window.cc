#include "window.h"

#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>

#include <absl/memory/memory.h>

const int kGLMajor = 3, kGLMinor = 0, kSamples = 0, kStencilBits = 8;

Window::Window() {}

Window::~Window() {
  glfwTerminate();
}

bool Window::isopen() {
  assert(m_window);
  return !glfwWindowShouldClose(m_window);
}

Error&& Window::Initialize(int width, int height) {
  if (!glfwInit())
    return Error::New("failed to initialize GLFW");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLMajor);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLMinor);

  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_DEPTH_BITS, 0);
  glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);

  m_window = glfwCreateWindow(width, height, "uterm", nullptr, nullptr);
  if (m_window == nullptr)
    return Error::New("failed to create window via GLFW");

  glfwMakeContextCurrent(m_window);

  if (gl3wInit())
    return Error::New("failed to initialize OpenGL");

  if (!gl3wIsSupported(kGLMajor, kGLMinor))
    return Error::New("failed to ensure OpenGL >=3.0 is supported");

  glfwGetFramebufferSize(m_window, &m_width, &m_height);

  glViewport(0, 0, m_width, m_height);
  glClearColor(1, 1, 1, 1);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  m_interface.reset(GrGLCreateNativeInterface());
  m_context = GrContext::MakeGL(m_interface.get());
  if (m_context == nullptr)
    return Error::New("failed to create GrContext");


  GrGLint buffer;
  GR_GL_GetIntegerv(m_interface.get(), GR_GL_FRAMEBUFFER_BINDING, &buffer);

  GrGLFramebufferInfo info;
  info.fFBOID = static_cast<GrGLuint>(buffer);
  m_target = absl::make_unique<GrBackendRenderTarget>(m_width, m_height, kSamples, kStencilBits,
                   kSkia8888_GrPixelConfig, info);

  SkSurfaceProps props{SkSurfaceProps::kLegacyFontHost_InitType};

  m_surface = SkSurface::MakeFromBackendRenderTarget(m_context.get(), *m_target,
                                                     kBottomLeft_GrSurfaceOrigin,
                                                     nullptr, &props);

  return Error::New();
}

void Window::Draw() {
  canvas()->flush();

  glfwSwapBuffers(m_window);
  glfwPollEvents();
}
