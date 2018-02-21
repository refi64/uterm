uterm
=====

uterm is a WIP terminal emulator, written in C++11 using Skia, libtsm, and GLFW.

Supported platforms
*******************

Currently uterm has only been tested on Linux, though it should also work on OSX. It
*may* also work on BSD; anyone willing to test it should be able to just edit
``fbuildroot.py`` to change all instances of ``{'linux'}`` to ``{'linux', 'bsd'}``.

Dependencies
************

- GLFW.
- OpenGL and EGL. These should come by default with your Linux mesa installation.
- Freetype2 and Fontconfig.

Building
********

You need `Fbuild <https://github.com/felix-lang/fbuild>`_ version 0.3 RC 2 or greater.
(Just using the *master* branch should be good enough.) Run::

  $ fbuild

to do a full build. If you want to do a release build, instead use::

  $ fbuild --release

Note that the initial build will take quite a while, as it will be building the entire
Skia library, which is pretty huge.

If you're concerned about size, a debug build is 73MB, and a release build is only 6MB
(largely thanks to LTO).
