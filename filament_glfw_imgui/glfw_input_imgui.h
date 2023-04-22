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
// Send GLFW input events to the ImGui GLFW backend.
//
// See glfw_input.h for usage.
// See filament_glfw_imgui.h for an integrated, working example.
//
// NOTE: this file isn't part of the google/filament release. It is part of
// ambrusc/filament-glfw-imgui.
//

#ifndef GLFW_INPUT_IMGUI_H_
#define GLFW_INPUT_IMGUI_H_

#include <imgui/backends/imgui_impl_glfw.h>
#include <imgui/imgui.h>

namespace glfw_input {

// Sends all input events to the ImGui GLFW backend.
class ToImGui {
 public:
  void ClearEvents() {}  // No state in this handler, so nothing to do.
  bool OnGlfwWindowFocus(GLFWwindow* window, int focused) {
    ImGui_ImplGlfw_WindowFocusCallback(window, focused);
    return ImGui::GetIO().WantCaptureMouse;
  }
  bool OnGlfwCursorEnter(GLFWwindow* window, int entered) {
    ImGui_ImplGlfw_CursorEnterCallback(window, entered);
    return ImGui::GetIO().WantCaptureMouse;
  }
  bool OnGlfwCursorPos(GLFWwindow* window, double x, double y) {
    ImGui_ImplGlfw_CursorPosCallback(window, x, y);
    return ImGui::GetIO().WantCaptureMouse;
  }
  bool OnGlfwMouseButton(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    return ImGui::GetIO().WantCaptureMouse;
  }
  bool OnGlfwScroll(GLFWwindow* window, double xoffset, double yoffset) {
    ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
    return ImGui::GetIO().WantCaptureMouse;
  }
  bool OnGlfwKey(GLFWwindow* window, int key, int scancode, int action,
                 int mods) {
    ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
    return ImGui::GetIO().WantCaptureKeyboard;
  }
  bool OnGlfwChar(GLFWwindow* window, unsigned int c) {
    ImGui_ImplGlfw_CharCallback(window, c);
    return ImGui::GetIO().WantCaptureKeyboard;
  }
};

// So you can write 'glfw_input::WithImGui' instead of
// 'glfw_input::Input<glfw_input::ToImGui>'
using WithImGui = Handler<ToImGui>;

}  // namespace glfw_input

#endif  // GLFW_INPUT_IMGUI_H_
