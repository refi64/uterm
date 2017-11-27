#include "keys.h"

#include <xkbcommon/xkbcommon-keysyms.h>
#include <GLFW/glfw3.h>

uint32 GlfwKeyToXkbKeysym(int key) {
  bool numlock = true;

  if (key >= GLFW_KEY_SPACE && key <= GLFW_KEY_GRAVE_ACCENT) {
    // Both GLFW and xkbcommon use the ASCII code as the key #.
    return key;
  } else if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25) {
    return (key - GLFW_KEY_F1) + XKB_KEY_F1;
  } else if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9) {
    if (numlock) {
      // XXX: Hardcode number pad directional keys.
      switch (key) {
      case GLFW_KEY_KP_0: return XKB_KEY_KP_Insert;
      case GLFW_KEY_KP_1: return XKB_KEY_KP_End;
      case GLFW_KEY_KP_2: return XKB_KEY_KP_Down;
      case GLFW_KEY_KP_3: return XKB_KEY_KP_Page_Down;
      case GLFW_KEY_KP_4: return XKB_KEY_KP_Left;
      case GLFW_KEY_KP_5: break;
      case GLFW_KEY_KP_6: return XKB_KEY_KP_Right;
      case GLFW_KEY_KP_7: return XKB_KEY_KP_Home;
      case GLFW_KEY_KP_8: return XKB_KEY_KP_Up;
      case GLFW_KEY_KP_9: return XKB_KEY_KP_Page_Up;
      default: break;
      }
    }

    return (key - GLFW_KEY_KP_0) + XKB_KEY_KP_0;
  } else {
    switch (key) {
    case GLFW_KEY_ESCAPE: return XKB_KEY_Escape;
    case GLFW_KEY_ENTER: return XKB_KEY_Return;
    case GLFW_KEY_TAB: return XKB_KEY_Tab;
    case GLFW_KEY_BACKSPACE: return XKB_KEY_BackSpace;
    case GLFW_KEY_INSERT: return XKB_KEY_Insert;
    case GLFW_KEY_DELETE: return XKB_KEY_Delete;
    case GLFW_KEY_RIGHT: return XKB_KEY_Right;
    case GLFW_KEY_LEFT: return XKB_KEY_Left;
    case GLFW_KEY_DOWN: return XKB_KEY_Down;
    case GLFW_KEY_UP: return XKB_KEY_Up;
    case GLFW_KEY_PAGE_UP: return XKB_KEY_Page_Up;
    case GLFW_KEY_PAGE_DOWN: return XKB_KEY_Page_Down;
    case GLFW_KEY_HOME: return XKB_KEY_Home;
    case GLFW_KEY_END: return XKB_KEY_End;
    case GLFW_KEY_CAPS_LOCK: return XKB_KEY_Caps_Lock;
    case GLFW_KEY_SCROLL_LOCK: return XKB_KEY_Scroll_Lock;
    case GLFW_KEY_NUM_LOCK: return XKB_KEY_Num_Lock;
    case GLFW_KEY_PRINT_SCREEN: return XKB_KEY_Print;
    case GLFW_KEY_PAUSE: return XKB_KEY_Pause;

    case GLFW_KEY_KP_DECIMAL: return numlock ? XKB_KEY_KP_Delete : XKB_KEY_KP_Decimal;
    case GLFW_KEY_KP_DIVIDE: return XKB_KEY_KP_Divide;
    case GLFW_KEY_KP_MULTIPLY: return XKB_KEY_KP_Multiply;
    case GLFW_KEY_KP_SUBTRACT: return XKB_KEY_KP_Subtract;
    case GLFW_KEY_KP_ADD: return XKB_KEY_KP_Add;
    case GLFW_KEY_KP_ENTER: return XKB_KEY_KP_Enter;
    case GLFW_KEY_KP_EQUAL: return XKB_KEY_KP_Equal;

    case GLFW_KEY_LEFT_SHIFT: return XKB_KEY_Shift_L;
    case GLFW_KEY_LEFT_CONTROL: return XKB_KEY_Control_L;
    case GLFW_KEY_LEFT_ALT: return XKB_KEY_Alt_L;
    case GLFW_KEY_LEFT_SUPER: return XKB_KEY_Super_L;
    case GLFW_KEY_RIGHT_SHIFT: return XKB_KEY_Shift_R;
    case GLFW_KEY_RIGHT_CONTROL: return XKB_KEY_Control_R;
    case GLFW_KEY_RIGHT_ALT: return XKB_KEY_Alt_R;
    case GLFW_KEY_RIGHT_SUPER: return XKB_KEY_Super_R;

    case GLFW_KEY_MENU: return XKB_KEY_Menu;

    case GLFW_KEY_UNKNOWN: case GLFW_KEY_WORLD_1: case GLFW_KEY_WORLD_2: default:
      return XKB_KEY_NoSymbol;
    }
  }
}
