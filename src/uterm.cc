#include "uterm.h"

#include <GrBackendSurface.h>
#include <GrContext.h>
#include <SkCanvas.h>
#include <SkTypeface.h>
#include <SkSurface.h>
#include <gl/GrGLInterface.h>
#include <gl/GrGLUtil.h>

#include <memory>

#include <assert.h>
#include <stdio.h>


const int kSamples = 0, kStencilBits = 8;


int main() {
  if (!glfwInit()) {
    puts("glfwInit failed");
    return 1;
  }
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

  glfwWindowHint(GLFW_RED_BITS, 8);
  glfwWindowHint(GLFW_GREEN_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_BLUE_BITS, 8);
  glfwWindowHint(GLFW_DOUBLEBUFFER, 1);
  glfwWindowHint(GLFW_DEPTH_BITS, 0);
  glfwWindowHint(GLFW_STENCIL_BITS, 8);

  GLFWwindow* window = glfwCreateWindow(800, 600, "uterm", nullptr, nullptr);
  if (!window) {
    puts("glfwCreateWindow failed");
    return 1;
  }

  glfwMakeContextCurrent(window);

  if (gl3wInit()) {
    puts("gl3wInit failed");
    return 1;
  }

  if (!gl3wIsSupported(3, 3)) {
    puts("gl3wIsSupported failed");
    return 1;
  }

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glViewport(0, 0, width, height);

  /* glViewport(0, 0, 800, 600); */
  glClearColor(1, 1, 1, 1);
  glClearStencil(0);
  glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

  sk_sp<const GrGLInterface> interface{GrGLCreateNativeInterface()};
  sk_sp<GrContext> grContext{GrContext::MakeGL(interface.get())};
  assert(grContext);

  GrGLint buffer;
  GR_GL_GetIntegerv(interface.get(), GR_GL_FRAMEBUFFER_BINDING, &buffer);

  GrGLFramebufferInfo info;
  info.fFBOID = (GrGLuint) buffer;
  GrBackendRenderTarget target{width, height, kSamples, kStencilBits, kSkia8888_GrPixelConfig,
                               info};

  SkSurfaceProps props{SkSurfaceProps::kLegacyFontHost_InitType};

  sk_sp<SkSurface> surface{SkSurface::MakeFromBackendRenderTarget(grContext.get(),
                                                                  target,
                                                                  kBottomLeft_GrSurfaceOrigin,
                                                                  nullptr, &props)};
  assert(surface);

  SkCanvas* canvas = surface->getCanvas();

  sk_sp<SkTypeface> robotoMono{SkTypeface::MakeFromName("Roboto Mono", SkFontStyle{})};

  SkPaint paint;
  paint.setColor(SK_ColorBLACK);
  paint.setTextSize(20);
  paint.setAntiAlias(true);
  paint.setTypeface(robotoMono);
  sk_sp<SkSurface> cpuSurface{SkSurface::MakeRaster(canvas->imageInfo())};

  while (!glfwWindowShouldClose(window)) {
    canvas->clear(SK_ColorWHITE);
    canvas->drawText("Hello, world!", 13, SkIntToScalar(100), SkIntToScalar(100), paint);

    canvas->save();
    canvas->restore();
    canvas->flush();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwTerminate();

  return 0;
}
