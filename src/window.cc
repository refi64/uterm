#include "window.h"
#include "terminal.h"
#include "keys.h"

#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>

#include <absl/memory/memory.h>

const int kGLMajor = 3, kGLMinor = 3, kPreferredSamples = 8, kStencilBits = 8;

Window::Window() {}

Window::~Window() {
  m_surface.reset();
  m_target.reset();
  m_context.reset();
  m_interface.reset();
  m_gl.reset();

  glfwSetCursor(m_window, nullptr);
  glfwDestroyCursor(m_cursor);
  glfwTerminate();
}

void Window::set_key_cb(KeyCb key_cb) { m_key_cb = key_cb; }
void Window::set_char_cb(CharCb char_cb) { m_char_cb = char_cb; }
void Window::set_resize_cb(ResizeCb resize_cb) { m_resize_cb = resize_cb; }
void Window::set_selection_cb(SelectionCb selection_cb) { m_selection_cb = selection_cb; }
void Window::set_scroll_cb(ScrollCb scroll_cb) { m_scroll_cb = scroll_cb; }

bool Window::isopen() {
  assert(m_window);
  return !glfwWindowShouldClose(m_window);
}

Error Window::Initialize(int width, int height, bool hwaccel, int vsync, const Theme& theme) {
  m_hwaccel = hwaccel;
  m_theme = &theme;

  glfwSetErrorCallback([](int ec, const char *err) {
    fmt::print("GLFW error: {}\n", err);
  });

  if (!glfwInit())
    return Error::New("failed to initialize GLFW");

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, kGLMajor);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, kGLMinor);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_ALPHA_BITS, 8);
  glfwWindowHint(GLFW_DEPTH_BITS, 0);
  glfwWindowHint(GLFW_STENCIL_BITS, kStencilBits);
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_SAMPLES, kPreferredSamples);
  #ifdef GLFW_TRANSPARENT_FRAMEBUFFER
  glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, 1);
  #endif

  m_window = glfwCreateWindow(width, height, "uterm", nullptr, nullptr);
  if (m_window == nullptr)
    return Error::New("failed to create window via GLFW");

  glfwMakeContextCurrent(m_window);
  glfwSwapInterval(vsync);

  glfwSetInputMode(m_window, GLFW_STICKY_KEYS, 1);
  glfwSetWindowUserPointer(m_window, static_cast<void*>(this));
  glfwSetKeyCallback(m_window, StaticKeyCallback);
  glfwSetCharCallback(m_window, StaticCharCallback);
  glfwSetWindowSizeCallback(m_window, StaticWinResizeCallback);
  glfwSetFramebufferSizeCallback(m_window, StaticFbResizeCallback);
  glfwSetMouseButtonCallback(m_window, StaticMouseCallback);
  glfwSetScrollCallback(m_window, StaticScrollCallback);

  m_cursor = glfwCreateStandardCursor(GLFW_IBEAM_CURSOR);
  glfwSetCursor(m_window, m_cursor);

  glfwGetFramebufferSize(m_window, &m_fb_width, &m_fb_height);

  if (m_hwaccel) {
    glViewport(0, 0, m_fb_width, m_fb_height);
    glClearColor(1, 1, 1, 0);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    m_interface = GrGLMakeNativeInterface();
    m_context = GrContext::MakeGL(m_interface);
    if (m_context == nullptr)
      return Error::New("failed to create GrContext");

    GrGLint buffer;
    GR_GL_GetIntegerv(m_interface.get(), GR_GL_FRAMEBUFFER_BINDING, &buffer);
    m_info.fFBOID = static_cast<GrGLuint>(buffer);
    // XXX
    m_info.fFormat = GR_GL_RGBA8;
  } else {
    if (auto err = m_gl->Initialize(m_fb_width, m_fb_height)) {
      return err;
    }
  }

  if (auto err = CreateSurface()) {
    return err;
  }

  return Error::New();
}

string Window::ClipboardRead() {
  const char *clip = glfwGetClipboardString(m_window);
  return clip != nullptr ? clip : "";
}

void Window::ClipboardWrite(const string &str) {
  glfwSetClipboardString(m_window, str.c_str());
}

void Window::SetTitle(const string &title) {
  glfwSetWindowTitle(m_window, title.c_str());
}

void Window::DrawAndPoll(bool significant_redraw) {
  if (significant_redraw && !m_hwaccel) {
    SkPixmap pixmap;
    canvas()->flush();
    canvas()->peekPixels(&pixmap);

    m_gl->UpdateTextureData(pixmap.addr());
  }

  if (m_hwaccel)
    canvas()->flush();
  else
    m_gl->Draw();
  glfwSwapBuffers(m_window);

  if (m_hwaccel)
    // Lazy-updating is not used when hardware-accelerated, so always clear the canvas.
    canvas()->clear((*m_theme)[Colors::kBackground]);

  bool previous_selection_status = m_selection_active;
  glfwPollEvents();

  double mx, my;
  glfwGetCursorPos(m_window, &mx, &my);

  if (m_selection_active) {
    if (previous_selection_status) {
      m_selection_cb(Selection::kUpdate, mx, my);
    } else {
      m_selection_cb(Selection::kBegin, mx, my);
    }
  } else if (previous_selection_status) {
    m_selection_cb(Selection::kEnd, mx, my);
  }
}

Error Window::CreateSurface() {
  // XXX: proper color type detection
  SkColorType colortype = kRGBA_8888_SkColorType;

  if (m_hwaccel) {
    m_surface.reset();

    int samples = m_context->maxSurfaceSampleCountForColorType(colortype);
    m_target = absl::make_unique<GrBackendRenderTarget>(m_fb_width, m_fb_height, samples,
                                                        kStencilBits, m_info);

    SkSurfaceProps props{SkSurfaceProps::kLegacyFontHost_InitType};
    m_surface = SkSurface::MakeFromBackendRenderTarget(m_context.get(), *m_target,
                                                       kBottomLeft_GrSurfaceOrigin,
                                                       kRGBA_8888_SkColorType, nullptr,
                                                       &props);
  } else {
    auto info = SkImageInfo::Make(m_fb_width, m_fb_height, kRGBA_8888_SkColorType,
                                  kPremul_SkAlphaType);
    m_surface = SkSurface::MakeRaster(info);
  }

  if (m_surface == nullptr) {
    return Error::New("failed to create SkSurface");
  }

  canvas()->clear((*m_theme)[Colors::kBackground]);
  return Error::New();
}

void Window::StaticKeyCallback(GLFWwindow *glfw_window, int key, int scancode,
                               int action, int glfw_mods){
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

  if (action != GLFW_PRESS && action != GLFW_REPEAT) return;

  int mods = 0;
  if (glfw_mods & GLFW_MOD_SHIFT) mods |= KeyboardModifier::kShift;
  if (glfw_mods & GLFW_MOD_CONTROL) mods |= KeyboardModifier::kControl;
  if (glfw_mods & GLFW_MOD_ALT) mods |= KeyboardModifier::kAlt;
  if (glfw_mods & GLFW_MOD_SUPER) mods |= KeyboardModifier::kSuper;

  uint32 keysym = GlfwKeyToXkbKeysym(key);
  window->m_key_cb(keysym, mods);
}

void Window::StaticCharCallback(GLFWwindow *glfw_window, uint code) {
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
  window->m_char_cb(code);
}

void Window::StaticWinResizeCallback(GLFWwindow *glfw_window, int width, int height) {
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
  window->m_resize_cb(width, height);
}

void Window::StaticFbResizeCallback(GLFWwindow *glfw_window, int width, int height) {
  if (width == 0 || height == 0) {
    return;
  }

  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

  window->m_fb_width = width;
  window->m_fb_height = height;

  if (window->m_hwaccel)
    glViewport(0, 0, width, height);
  else
    window->m_gl->Resize(width, height);

  if (auto err = window->CreateSurface()) {
    err.Extend("in StaticResizeCallback").Print();
  }
}

void Window::StaticMouseCallback(GLFWwindow *glfw_window, int button, int action,
                                 int mods) {
  if (button != GLFW_MOUSE_BUTTON_LEFT) return;
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));

  if (action == GLFW_PRESS) {
    window->m_selection_active = true;
  } else if (action == GLFW_RELEASE) {
    window->m_selection_active = false;
  }
}

void Window::StaticScrollCallback(GLFWwindow *glfw_window, double xoffset,
                                  double yoffset) {
  Window *window = static_cast<Window*>(glfwGetWindowUserPointer(glfw_window));
  if (yoffset > 0) {
    window->m_scroll_cb(ScrollDirection::kUp, yoffset);
  } else if (yoffset < 0) {
    window->m_scroll_cb(ScrollDirection::kDown, -yoffset);
  }
}
