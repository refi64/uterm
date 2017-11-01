#include "window.h"
#include "terminal.h"

#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>

#include <absl/memory/memory.h>

// Grrrr...
#define Window XWindow
#include <X11/XKBlib.h>
#undef Window

const int kGLMajor = 3, kGLMinor = 0, kSamples = 4, kStencilBits = 8;

Window::Window() {}

Window::~Window() {
  glfwTerminate();
}

void Window::set_key_cb(KeyCb key_cb) { m_key_cb = key_cb; }
void Window::set_char_cb(CharCb char_cb) { m_char_cb = char_cb; }
void Window::set_resize_cb(ResizeCb resize_cb) { m_resize_cb = resize_cb; }

bool Window::isopen() {
  assert(m_window);
  return !glfwWindowShouldClose(m_window);
}

Error Window::Initialize(int width, int height) {
  if (!glfwInit())
    return Error::New("failed to initialize GLFW");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLMajor);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLMinor);

  // Set stuff up like Skia wants it.
  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_DEPTH_BITS, 0);
  glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);
  glfwWindowHint(GLFW_SAMPLES, kSamples);

  m_window = glfwCreateWindow(width, height, "uterm", nullptr, nullptr);
  if (m_window == nullptr)
    return Error::New("failed to create window via GLFW");

  glfwMakeContextCurrent(m_window);

  glfwSetInputMode(m_window, GLFW_STICKY_KEYS, 1);
  glfwSetWindowUserPointer(m_window, static_cast<void*>(this));
  glfwSetKeyCallback(m_window, StaticKeyCallback);
  glfwSetCharCallback(m_window, StaticCharCallback);
  glfwSetFramebufferSizeCallback(m_window, StaticResizeCallback);

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

  m_info.fFBOID = static_cast<GrGLuint>(buffer);

  if (auto err = CreateSurface()) {
    return err;
  } else {
    return Error::New();
  }
}

void Window::Draw() {
  canvas()->flush();
  glfwSwapBuffers(m_window);

  glfwPollEvents();
}

Error Window::CreateSurface() {
  m_surface.reset();

  m_target = absl::make_unique<GrBackendRenderTarget>(m_width, m_height, kSamples,
                                                      kStencilBits,
                                                      kSkia8888_GrPixelConfig,
                                                      m_info);

  SkSurfaceProps props{SkSurfaceProps::kLegacyFontHost_InitType};

  m_surface = SkSurface::MakeFromBackendRenderTarget(m_context.get(), *m_target,
                                                     kBottomLeft_GrSurfaceOrigin,
                                                     nullptr, &props);
  if (m_surface == nullptr)
    return Error::New("failed to create SkSurface");

  return Error::New();
}

void Window::StaticKeyCallback(GLFWwindow *glfw_window, int key, int scancode,
                               int action, int glfw_mods){
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

  if (action != GLFW_PRESS) return;

  int mods = 0;
  if (glfw_mods & GLFW_MOD_SHIFT) mods |= KeyboardModifier::kShift;
  if (glfw_mods & GLFW_MOD_CONTROL) mods |= KeyboardModifier::kControl;
  if (glfw_mods & GLFW_MOD_ALT) mods |= KeyboardModifier::kAlt;
  if (glfw_mods & GLFW_MOD_SUPER) mods |= KeyboardModifier::kSuper;

  uint32 keysym = XkbKeycodeToKeysym(XOpenDisplay(nullptr), scancode, 0,
                                     mods & KeyboardModifier::kShift ? 1 : 0);
  window->m_key_cb(keysym, mods);
}

void Window::StaticCharCallback(GLFWwindow *glfw_window, uint code) {
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
  window->m_char_cb(code);
}

void Window::StaticResizeCallback(GLFWwindow *glfw_window, int width, int height) {
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

  window->m_width = width;
  window->m_height = height;
  glViewport(0, 0, width, height);

  if (auto err = window->CreateSurface()) {
    err.Extend("in StaticResizeCallback").Print();
  }

  window->m_resize_cb(width, height);
}
