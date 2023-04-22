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

#ifndef GLFW_INPUT_IMPL_H_
#define GLFW_INPUT_IMPL_H_

namespace glfw_input {

template <typename Mask>
bool HasBits(Mask mask, int bits) {
  return (mask.value & bits) == bits;
}

template <typename Mask>
void SetBits(Mask& mask, int bits) {
  mask.value |= bits;
}

template <typename Mask>
void ClearBits(Mask& mask, int bits) {
  mask.value &= ~bits;
}

inline int MouseButtonMask::ButtonToBits(int glfw_mouse_button) {
  return 1 << glfw_mouse_button;
}

inline bool MouseButtonMask::HasGlfwButton(int glfw_mouse_button) const {
  return HasBits(*this, ButtonToBits(glfw_mouse_button));
}

inline bool KeyboardState::IsPressed(int32_t key) const {
  return PressedEventIndex(key);
}

inline int64_t KeyboardState::PressedEventIndex(int32_t key) const {
  return key < pressed_event_index_.size() ? pressed_event_index_[key] : 0;
}

inline void KeyboardState::SetKeyEventIndex(int32_t key, uint64_t event_index) {
  if (key < pressed_event_index_.size()) {
    pressed_event_index_[key] = event_index;
  }
}

inline int KeyboardState::Axis(int32_t key_minus, int32_t key_plus) const {
  const int state_minus = PressedEventIndex(key_minus);
  const int state_plus = PressedEventIndex(key_plus);
  if (state_minus > state_plus) {
    return -1;
  } else if (state_plus > state_minus) {
    return 1;
  }
  return 0;
}

template <typename Child>
inline Handler<Child>::Handler(Child&& child) : child_(child) {}

template <typename Child>
inline void Handler<Child>::ClearEvents() {
  child_.ClearEvents();
  state_.events.clear();
  state_.all_events.clear();
}

template <typename Child>
inline bool Handler<Child>::OnGlfwWindowFocus(GLFWwindow* window, int focused) {
  const bool child_wants_capture = child_.OnGlfwWindowFocus(window, focused);

  ++state_.event_index;

  if (focused) {
    state_.mouse_x = kDoubleInf;
    state_.mouse_y = kDoubleInf;
  }

  Event e = {Event::kFocus, window};
  e.child_wants_capture = child_wants_capture;
  e.focused = focused;

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwCursorEnter(GLFWwindow* window, int entered) {
  const bool child_wants_capture = child_.OnGlfwCursorEnter(window, entered);

  ++state_.event_index;

  Event e = {Event::kEnter, window};
  e.child_wants_capture = child_wants_capture;
  e.entered = entered;

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwCursorPos(GLFWwindow* window, double x,
                                            double y) {
  const bool child_wants_capture = child_.OnGlfwCursorPos(window, x, y);

  ++state_.event_index;

  double xoffset = 0;
  double yoffset = 0;
  if (state_.mouse_x != kDoubleInf && state_.mouse_y != kDoubleInf) {
    xoffset = x - state_.mouse_x;
    yoffset = y - state_.mouse_y;
  }
  state_.mouse_x = x;
  state_.mouse_y = y;

  Event e = {Event::kCursorPos, window};
  e.child_wants_capture = child_wants_capture;
  e.cursor_pos = {
      x, y, xoffset, yoffset, state_.mouse_buttons, state_.mod_keys};

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwMouseButton(GLFWwindow* window, int button,
                                              int action, int mods) {
  const bool child_wants_capture =
      child_.OnGlfwMouseButton(window, button, action, mods);

  ++state_.event_index;

  if (action == GLFW_PRESS) {
    SetBits(state_.mouse_buttons, MouseButtonMask::ButtonToBits(button));
  } else if (action == GLFW_RELEASE) {
    ClearBits(state_.mouse_buttons, MouseButtonMask::ButtonToBits(button));
  }

  Event e = {Event::kMouseButton, window};
  e.child_wants_capture = child_wants_capture;
  e.mouse_button = {button, action, state_.mod_keys};

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwScroll(GLFWwindow* window, double xoffset,
                                         double yoffset) {
  const bool child_wants_capture =
      child_.OnGlfwScroll(window, xoffset, yoffset);

  ++state_.event_index;

  Event e = {Event::kScroll, window};
  e.child_wants_capture = child_wants_capture;
  e.scroll = {xoffset, yoffset, state_.mod_keys};

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwKey(GLFWwindow* window, int key, int scancode,
                                      int action, int mods) {
  const bool child_wants_capture =
      child_.OnGlfwKey(window, key, scancode, action, mods);

  ++state_.event_index;

  switch (key) {
    case GLFW_KEY_LEFT_SHIFT:
    case GLFW_KEY_RIGHT_SHIFT:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_SHIFT);
      if (action == GLFW_RELEASE) ClearBits(state_.mod_keys, GLFW_MOD_SHIFT);
      break;
    case GLFW_KEY_LEFT_CONTROL:
    case GLFW_KEY_RIGHT_CONTROL:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_CONTROL);
      if (action == GLFW_RELEASE) ClearBits(state_.mod_keys, GLFW_MOD_CONTROL);
      break;
    case GLFW_KEY_LEFT_ALT:
    case GLFW_KEY_RIGHT_ALT:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_ALT);
      if (action == GLFW_RELEASE) ClearBits(state_.mod_keys, GLFW_MOD_ALT);
      break;
    case GLFW_KEY_LEFT_SUPER:
    case GLFW_KEY_RIGHT_SUPER:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_SUPER);
      if (action == GLFW_RELEASE) ClearBits(state_.mod_keys, GLFW_MOD_SUPER);
      break;
    case GLFW_KEY_NUM_LOCK:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_NUM_LOCK);
      if (action == GLFW_RELEASE) ClearBits(state_.mod_keys, GLFW_MOD_NUM_LOCK);
      break;
    case GLFW_KEY_CAPS_LOCK:
      if (action == GLFW_PRESS) SetBits(state_.mod_keys, GLFW_MOD_CAPS_LOCK);
      if (action == GLFW_RELEASE)
        ClearBits(state_.mod_keys, GLFW_MOD_CAPS_LOCK);
      break;
  }

  Event e = {Event::kKey, window};
  e.child_wants_capture = child_wants_capture;
  e.key = {key, scancode, action, state_.mod_keys};

  if (!child_wants_capture) {
    if (action == GLFW_PRESS) {
      state_.keys.SetKeyEventIndex(key, state_.event_index);
    } else if (action == GLFW_RELEASE) {
      state_.keys.SetKeyEventIndex(key, 0);
    }
    state_.events.push_back(e);
  }

  if (action == GLFW_PRESS) {
    state_.all_keys.SetKeyEventIndex(key, state_.event_index);
  } else if (action == GLFW_RELEASE) {
    state_.all_keys.SetKeyEventIndex(key, 0);
  }
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename Child>
inline bool Handler<Child>::OnGlfwChar(GLFWwindow* window, unsigned int c) {
  const bool child_wants_capture = child_.OnGlfwChar(window, c);

  ++state_.event_index;

  Event e = {Event::kChar, window};
  e.child_wants_capture = child_wants_capture;
  e.charval = c;

  if (!child_wants_capture) state_.events.push_back(e);
  state_.all_events.push_back(e);

  return child_wants_capture;
}

template <typename HandlerT>
void GlfwAttachInputCallbacksAndSetWindowUserPointer(const HandlerT& handler,
                                                     GLFWwindow& window) {
  glfwSetWindowUserPointer(&window, (void*)&handler);
  glfwSetWindowFocusCallback(&window, glfw_input::OnGlfwWindowFocus<HandlerT>);
  glfwSetCursorEnterCallback(&window, glfw_input::OnGlfwCursorEnter<HandlerT>);
  glfwSetCursorPosCallback(&window, glfw_input::OnGlfwCursorPos<HandlerT>);
  glfwSetMouseButtonCallback(&window, glfw_input::OnGlfwMouseButton<HandlerT>);
  glfwSetScrollCallback(&window, glfw_input::OnGlfwScroll<HandlerT>);
  glfwSetKeyCallback(&window, glfw_input::OnGlfwKey<HandlerT>);
  glfwSetCharCallback(&window, glfw_input::OnGlfwChar<HandlerT>);
}

}  // namespace glfw_input

#endif  // GLFW_INPUT_IMPL_H_
