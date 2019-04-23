include(ExternalProject)

ExternalProject_Add(gn_proj
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/gn"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/gn"
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  CONFIGURE_COMMAND
    ${CMAKE_COMMAND} -E env
    CC=${CMAKE_C_COMPILER} CXX=${CMAKE_CXX_COMPILER} LD=${CMAKE_CXX_COMPILER} AR=${CMAKE_AR}
    ${PYTHON_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/deps/gn/build/gen.py --out-path=.
    # Don't try to do static libs for this
    COMMAND sed -i "s/-static-libstdc++//g" build.ninja
  BUILD_COMMAND ${CMAKE_MAKE_PROGRAM} gn
  INSTALL_COMMAND "")

string(JOIN " " SKIA_CONFIGURE_ARGS
  skia_use_egl=true
  skia_use_expat=false
  skia_use_icu=false
  skia_use_libheif=false
  skia_use_libjpeg_turbo=false
  skia_use_libpng=false
  skia_use_libwebp=false
  skia_use_lua=false
  skia_use_system_freetype2=true
  skia_use_zlib=false
  skia_enable_atlas_text=false
  skia_enable_spirv_validation=false
  skia_enable_tools=false
  cc="${CMAKE_C_COMPILER}"
  cxx="${CMAKE_CXX_COMPILER}"
  ar="${CMAKE_AR}"
  # XXX: Try to make up for Skia including egl too soon
  extra_cflags=["-includeepoxy/gl.h","-includeepoxy/egl.h","-DMESA_EGL_NO_X11_HEADERS"])

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
  set(SKIA_CONFIGURE_ARGS "${SKIA_CONFIGURE_ARGS} is_debug=true")
elseif (${CMAKE_BUILD_TYPE} MATCHES Rel)
  set(SKIA_CONFIGURE_ARGS "${SKIA_CONFIGURE_ARGS} is_official_build=true")
  if (${CMAKE_BUILD_TYPE} MATCHES RelWithDebInfo)
    set(SKIA_CONFIGURE_ARGS "${SKIA_CONFIGURE_ARGS} extra_cflags+=[\"-g\"]")
  endif ()
endif ()

ExternalProject_Add(skia_proj
  DEPENDS gn_proj
  SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/deps/skia"
  BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/skia"
  BUILD_BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/skia/libskia.a"
  USES_TERMINAL_CONFIGURE TRUE
  USES_TERMINAL_BUILD TRUE
  CONFIGURE_COMMAND
    ${CMAKE_CURRENT_BINARY_DIR}/gn/gn gen .
    --root=${CMAKE_CURRENT_SOURCE_DIR}/deps/skia --args=${SKIA_CONFIGURE_ARGS}
  BUILD_COMMAND ${CMAKE_MAKE_PROGRAM}
  INSTALL_COMMAND "")

add_library(skia STATIC IMPORTED)
add_dependencies(skia skia_proj)
target_include_directories(skia INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/skia/include/config
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/skia/include/core
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/skia/include/gpu
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/skia/include/private
  ${CMAKE_CURRENT_SOURCE_DIR}/deps/skia/src/gpu)
set_property(TARGET skia PROPERTY IMPORTED_LOCATION
             ${CMAKE_CURRENT_BINARY_DIR}/skia/libskia.a)
target_link_libraries(skia INTERFACE ${CMAKE_DL_LIBS})
