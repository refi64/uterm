cmake_minimum_required(VERSION 3.8)
project(uterm LANGUAGES CXX)

set(OpenGL_GL_PREFERENCE GLVND)

if (NOT ${CMAKE_GENERATOR} STREQUAL Ninja)
  message(FATAL_ERROR "Ninja must be used as the build generator.")
endif ()

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  message(WARNING "Clang is recommended for best performance")
endif()

# Needed for GN
find_package(Python COMPONENTS Interpreter REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GLFW3 REQUIRED glfw3)
pkg_check_modules(EGL REQUIRED egl)
pkg_check_modules(EPOXY REQUIRED epoxy)
pkg_check_modules(LIBCONFUSE REQUIRED libconfuse)

if (UNIX AND NOT APPLE)
  pkg_check_modules(FONTCONFIG REQUIRED fontconfig)
  pkg_check_modules(FREETYPE REQUIRED freetype2)
  set(UNIX_FONT_STACK TRUE)
endif ()

add_compile_options(-fdiagnostics-color -Wno-everything -Wno-fatal-errors)

find_library(TCMALLOC NAMES tcmalloc)
if (NOT TCMALLOC)
  message(WARNING "tcmalloc is recommended for best performance.")
endif ()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(UTF8_TESTS OFF CACHE BOOL "")
set(UTF8_SAMPLES OFF CACHE BOOL "")

add_subdirectory(deps/abseil)
add_subdirectory(deps/fmt)
add_subdirectory(deps/libtsm)
add_subdirectory(deps/utfcpp)

include(cmake/BuildPhmap.cmake)
include(cmake/BuildSkia.cmake)

add_executable(uterm
  src/config.cc
  src/display.cc
  src/error.cc
  src/gl_manager.cc
  src/keys.cc
  src/main.cc
  src/pty.cc
  src/terminal.cc
  src/text.cc
  src/uterm.cc
  src/window.cc)
target_compile_features(uterm PUBLIC cxx_std_14)
target_include_directories(uterm PUBLIC
  ${GLFW3_INCLUDE_DIRS}
  ${EGL_INCLUDE_DIRS}
  ${EPOXY_INCLUDE_DIRS}
  ${LIBCONFUSE_INCLUDE_DIRS})
target_link_libraries(uterm
  absl::base
  absl::failure_signal_handler
  absl::hash
  absl::stacktrace
  absl::strings
  absl::symbolize
  fmt::fmt
  phmap
  skia
  libtsm::tsm
  utf8::cpp
  ${GLFW3_LIBRARIES}
  ${EGL_LIBRARIES}
  ${EPOXY_LIBRARIES}
  ${LIBCONFUSE_LIBRARIES}
  ${TCMALLOC})

if (UNIX_FONT_STACK)
  target_link_libraries(uterm
    ${FONTCONFIG_LIBRARIES}
    ${FREETYPE_LIBRARIES})
endif ()
