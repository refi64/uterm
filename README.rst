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

Configuration
*************

Configuration is done using `libConfuse <https://github.com/martinh/libconfuse>`_.
Here's an example config file (stored in ``$HOME/.config/uterm/uterm.conf``)

.. code-block:: c

  // C-style comments are supported

  // ***FONTS**

  // Set the default font size.
  font-defaults {
    size = 16
  }

  // By default uterm uses the system monospace font. Any fonts appearing here will
  // take priority in the order that they are specified in this file.

  // These can specify a size too if you want; it'll look just like above.

  // In this example, Roboto Mono is the #1 font. If any characters aren't available
  // in Roboto Mono, it'll fall back to Hack. After that, it will fall back to the system
  // monospace font.
  font "Roboto Mono" {}
  font Hack {}

  // Theming

  theme test {
    // You can use 0xRRGGBB style
    red = 0xFF0000
    // or 0xRRGGBBAA style
    background = 0xFFFFFFAA

    // Supported colors: black, red, green, yellow, blue, magenta, cyan, white
    // Prefix a color with bright_ (e.g. red_bright) to set the "bright" version of the color.
    // You can also set the default foreground and background color:

    foreground = 0x000000

    // If any of the mentioned colors are ommitted, they will use the versions from the default
    // theme.
  }

  // You can define multiple themes in this file. To switch between them, use current-theme:
  current-theme = test

  // If you omit it to set it to an empty string (""), the default theme will be used.
