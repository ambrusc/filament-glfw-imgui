// -----------------------------------------------------------------------------
// Copyright 2023 filament_glfw_imgui Library Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// -----------------------------------------------------------------------------

//
// Collect GLFW events for processing in your app, and optionally pass them
// to a another library (e.g. ImGui).
//
// See filament_glfw_imgui.h for an integrated, working example.
//
// NOTE: this file isn't part of the google/filament release. It is part of
// ambrusc/filament-glfw-imgui.
//
// Recommended Usage:
//
//   // Forward events to ImGui.
//   auto handler = glfw_input::WithImGui();
//   GlfwAttachInputCallbacksAndSetWindowUserPointer(handler, *glfw_window);
//
//   while (...) { // Your mainloop.
//     handler.ClearEvents();
//     glfwPollEvents();
//     const glfw_input::State& input = handler.state();
//
//     // Mouse drag example (with Shift key pressed).
//     for (const glfw_input::Event& event : input.events) {
//       switch (event.type) {
//         case Event::kCursorPos:
//           if (event.cursor_pos.buttons.HasGlfwButton(
//                                        GLFW_MOUSE_BUTTON_LEFT)) {
//             if (event.cursor_pos.mods.HasGlfwKey(GLFW_MOD_SHIFT)) {
//               const double dx = event.cursor_pos.xoffset;
//               const double dy = event.cursor_pos.yoffset;
//               // Use dx, dy...
//             }
//           }
//           break;
//       }
//     }
//
//     // WASD keyboard movement example.
//     const int move = input.keys.Axis(GLFW_KEY_S, GLFW_KEY_W);
//     const int strafe = input.keys.Axis(GLFW_KEY_A, GLFW_KEY_D);
//     // Use move, strafe...
//   }
//

#ifndef GLFW_INPUT_H_
#define GLFW_INPUT_H_

#include <GLFW/glfw3.h>

#include <cstdint>
#include <vector>

namespace glfw_input {

static constexpr double kDoubleInf = std::numeric_limits<double>::infinity();

// Converts GLFW mouse buttons values (0-7) to a bitmask and stores them.
struct MouseButtonMask {
  int value = 0;

  // Converts a button e.g. GLFW_MOUSE_BUTTON_LEFT to a bitmask.
  static int ButtonToBits(int glfw_mouse_button);

  // Returns 'true' if the button (e.g. GLFW_MOUSE_BUTTON_LEFT) bits are set.
  bool HasGlfwButton(int glfw_mouse_button) const;
};

// Stores GLFW modifier keys (e.g. GLFW_MOD_SHIFT) as a bitmask.
struct ModKeyMask {
  int value = 0;

  // Returns 'true' if the key (e.g. GLFW_KEY_...) bits are set.
  bool HasGlfwKey(int glfw_key) const;
};

// Slightly extended event structure that maps to GLFW input callbacks.
struct Event {
  static constexpr uint64_t kNone = 0;
  static constexpr uint64_t kFocus = 0x1;
  static constexpr uint64_t kEnter = 0x2;
  static constexpr uint64_t kCursorPos = 0x4;
  static constexpr uint64_t kMouseButton = 0x8;
  static constexpr uint64_t kScroll = 0x10;
  static constexpr uint64_t kKey = 0x20;
  static constexpr uint64_t kChar = 0x40;
  static constexpr uint64_t kAllEvents = uint64_t(-1);

  struct CursorPos {
    double x;
    double y;
    // Fields not present in GLFW (added in this library).
    double xoffset;
    double yoffset;
    MouseButtonMask buttons;  // buttons.HasGlfwButton(GLFW_MOUSE_BUTTON_...)
    ModKeyMask mods;          // GLFW_MOD_... bitmask.
  };

  struct MouseButton {
    int button;       // GLFW_MOUSE_BUTTON_...
    int action;       // GLFW_PRESS, etc.
    ModKeyMask mods;  // GLFW_MOD_... bitmask.
  };

  struct Scroll {
    double xoffset;
    double yoffset;
    // Fields not present in GLFW (added in this library).
    ModKeyMask mods;  // GLFW_MOD_... bitmask.
  };

  struct Key {
    int key;          // GLFW_KEY_...
    int scancode;     // As reported by GLFW.
    int action;       // GLFW_PRESS, etc.
    ModKeyMask mods;  // GLFW_MOD_... bitmask.
  };

  // One of the constexpr values above, indicates the active union field below.
  uint64_t type;

  // The window on which the event occurred.
  GLFWwindow* window;

  // Events are passed to a stack of handlers. If any handler in the stack wants
  // to capture this event, this flag is set. For instance, when using the
  // glfw_input::ToImGui handler, this flag is set if ImGui::GetIO() indicates
  // WantKeyboardCapture (for keyboard events) or WantMouseCapture (for mouse
  // events).
  bool child_wants_capture;

  // Data from a GLFW callback (and some additional niceties).
  union {
    int focused;  // GLFW_TRUE/FALSE (i.e. 1 or 0)
    int entered;  // GLFW_TRUE/FALSE (i.e. 1 or 0)
    CursorPos cursor_pos;
    MouseButton mouse_button;
    Scroll scroll;
    Key key;
    unsigned int charval;  // For text input, as reported by GLFW.
  };
};

// Tracks keyboard key pressed/released state.
//
// GLFW is able track keyboard state, but we want to respect ImGui's
// WantCaptureKeyboard/Mouse flags, so we do our own state tracking. As an added
// benefit we can do things like the key-axis mapping: see Axis(...).
class KeyboardState {
 public:
  KeyboardState(int32_t max_key) : pressed_event_index_(max_key + 1) {}

  // True if the key is currently pressed.
  // False if the key is released, or it's outside the tracked range.
  bool IsPressed(int32_t key) const;

  // Returns the event index when the given key was pressed.
  // 0 if the key is released, or it's outside the tracked range.
  int64_t PressedEventIndex(int32_t key) const;

  // Useful for binding two keys to an input axis (e.g. wasd).
  //  - Compares the event index of key_minus and key_plus.
  //  - Keys outside the tracked range are treated as released.
  //
  // Returns:
  //   -1 if 'key_minus' was pressed after 'key_plus'
  //    0 if both keys are released
  //    1 if 'key_plus' was pressed after 'key_minus'
  int Axis(int32_t key_minus, int32_t key_plus) const;

  // Set a key to the given event index.
  // No state change if the key is outside the tracked range.
  void SetKeyEventIndex(int32_t key, uint64_t event_index);

 private:
  // Stores the event index when the key was pressed, 0 if the key is released.
  std::vector<uint64_t> pressed_event_index_;
};

// Input state, usually provided every frame.
struct State {
  State() : keys(GLFW_KEY_LAST), all_keys(GLFW_KEY_LAST) {}

  // Incremented every time a new event is received.
  // If I've done the math right, at 120 frames/sec and 10 events/frame, this
  // would take almost half a million years to overflow.
  uint64_t event_index = 0;

  // Events and keyboard state that wasn't captured by a child handler.
  // Cleared by Input::ClearEvents().
  std::vector<Event> events;
  KeyboardState keys;

  // ALL Events and keyboard state, even if it was captured by a child handler.
  // (Use with caution). Cleared by Input::ClearEvents().
  std::vector<Event> all_events;
  KeyboardState all_keys;

  // ---------------------------------------------------------------------------
  // Below: fields not recommended for use in your app.
  // Use the equivalents populated on Event instead.
  //
  // WHY? These are rolling state variables that are updated per-event, and
  // by default, they reflect the input state after every glfwPollEvents (i.e.
  // at each frame boundary. However, frame boundaries are arbitrary -- they can
  // come anywhere in the event stream. If your app handles input event-by-event
  // rather than frame-by-frame, you can ensure correctness regardless of fps,
  // frame stutters, mouse focus, etc.
  // ---------------------------------------------------------------------------

  // We separately track keyboard modifier keys to make sure their handling
  // is cross-platform consistent. (Specifically, X11 seems to be misbehaving).
  // [bug] https://github.com/glfw/glfw/issues/1630
  // [feature] https://github.com/glfw/glfw/issues/2126
  // Mod keys reflect the physical input-device state, and as such, are
  // independent of input capture.
  ModKeyMask mod_keys = {};

  // ImGui also tracks mouse position/delta, but it doesn't update the delta
  // unless ImGui::NewFrame is called, which we can't rely on here. We do our
  // own mouse tracking instead. Inf indicates an invalid position (e.g. when
  // our app isn't currently being provided mouse information by the OS).
  double mouse_x = kDoubleInf;
  double mouse_y = kDoubleInf;

  // Mouse buttons reflect the physical input-device state, and as such, are
  // independent of input capture.
  MouseButtonMask mouse_buttons = {};
};

// A no-op input handler.
class NoOpHandler {
 public:
  void ClearEvents() {}
  bool OnGlfwWindowFocus(GLFWwindow*, int) { return false; }
  bool OnGlfwCursorEnter(GLFWwindow*, int) { return false; }
  bool OnGlfwCursorPos(GLFWwindow*, double, double) { return false; }
  bool OnGlfwMouseButton(GLFWwindow*, int, int, int) { return false; }
  bool OnGlfwScroll(GLFWwindow*, double, double) { return false; }
  bool OnGlfwKey(GLFWwindow*, int, int, int, int) { return false; }
  bool OnGlfwChar(GLFWwindow*, unsigned int) { return false; }
};

// Stateful event handling for GLFW.
//  - Sends GLFW input callbacks to Handler (e.g. imgui_glfw::Input).
//  - Converts GLFW input callbacks to an event list.
//  - Tracks basic input state for the current frame.
template <typename Child = NoOpHandler>
class Handler {
 public:
  Handler(Child&& child = {});

  // Call this to clear the current events before glfwPollEvents.
  void ClearEvents();

  // The sub-handler.
  const Child& child() const { return child_; }
  Child& child() { return child_; }

  // Current keyboard and mouse state.
  const State& state() const { return state_; }
  State& state() { return state_; }

  // GLFW callback handlers.
  //
  // Return: 'true' if the child handler wants to capture an input event
  // (i.e. parent handlers should treat the event as having been claimed). For
  // instance the glfw_imgui_input::Input returns 'true' if
  // ImGui::GetIO().WantCaptureKeyboard/Mouse is true.
  //
  // These are called by GLFW if GlfwAttachCallbacksAndSetWindowUserPointer(...)
  // was called. Otherwise, you can call these yourself to track state in this
  // class.
  bool OnGlfwWindowFocus(GLFWwindow* window, int focused);
  bool OnGlfwCursorEnter(GLFWwindow* window, int entered);
  bool OnGlfwCursorPos(GLFWwindow* window, double x, double y);
  bool OnGlfwMouseButton(GLFWwindow* window, int button, int action, int mods);
  bool OnGlfwScroll(GLFWwindow* window, double xoffset, double yoffset);
  bool OnGlfwKey(GLFWwindow* window, int key, int scancode, int action,
                 int mods);
  bool OnGlfwChar(GLFWwindow* window, unsigned int c);

 private:
  Child child_;       // Sub-handler.
  State state_ = {};  // Input state.
};

//------------------------------------------------------------------------------
// Toplevel GLFW callback handlers
//
// You can call GlfwAttachCallbacksAndSetWindowUserPointer(...) to set
// glfwSetWindowUserPointer to your handler and attach input callbacks in one
// shot, or you can attach them individually by calling
// glfwSet...Callback(window, OnGlfw...<YourHandler>()).
//------------------------------------------------------------------------------

// Attaches GLFW callbacks and sets glfwSetWindowUserPointer to the handler.
template <typename HandlerT>
void GlfwAttachInputCallbacksAndSetWindowUserPointer(const HandlerT& handler,
                                                     GLFWwindow& window);

template <typename HandlerT>
void OnGlfwWindowFocus(GLFWwindow* window, int focused) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwWindowFocus(window, focused);
}

template <typename HandlerT>
void OnGlfwCursorEnter(GLFWwindow* window, int entered) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwCursorEnter(window, entered);
}

template <typename HandlerT>
void OnGlfwCursorPos(GLFWwindow* window, double x, double y) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwCursorPos(window, x, y);
}

template <typename HandlerT>
void OnGlfwMouseButton(GLFWwindow* window, int button, int action, int mods) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwMouseButton(window, button, action, mods);
}

template <typename HandlerT>
void OnGlfwScroll(GLFWwindow* window, double xoffset, double yoffset) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwScroll(window, xoffset, yoffset);
}

template <typename HandlerT>
void OnGlfwKey(GLFWwindow* window, int key, int scancode, int action,
               int mods) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwKey(window, key, scancode, action, mods);
}

template <typename HandlerT>
void OnGlfwChar(GLFWwindow* window, unsigned int c) {
  auto handler = (HandlerT*)glfwGetWindowUserPointer(window);
  handler->OnGlfwChar(window, c);
}

}  // namespace glfw_input

#include "filament_glfw_imgui/glfw_input_impl.h"

#endif  // GLFW_INPUT_H_
