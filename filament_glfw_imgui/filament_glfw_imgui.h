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
// Sets up and attaches a Filament engine to a GLFW window with ImGui UI.
//
// NOTE: this file isn't part of the google/filament release. It is part of
// ambrusc/filament-glfw-imgui.
//
// Recommended usage:
//
//   // Load filament_imgui.filamat (either from resource or file).
//   const uint8_t* const imgui_mat_data = // ...
//   const size_t imgui_mat_size =
//
//   GLFWwindow* window = glfwCreateWindow(...);
//
//   // Caller can pass any ostream to 'log', or 'nullptr' to squelch logging.
//   app = filament_glfw_imgui::App(window, imgui_mat_data, imgui_mat_size)
//
//   // Init() will return 'false' and log a message if 'window' is null, or
//   // if any of the initialization steps failed.
//   if (!app.Init()) return;
//
//   // Set up your Filament scene and add your ImGui fonts here.
//
//   while (app.Run()) {
//     glfw_input::Input& input = *app.PollEvents();
//
//     // Optionally add more fonts (e.g. filament_imgui::AddFont(...)) here.
//
//     app.BeginUiFrame();
//
//     // Do updates, draw UI with ImGui commands here.
//
//     app.EndUiFrame();
//     if (app.BeginRender()) {
//
//       // Render your Filament views here.
//
//       app.renderer()->render(app.ui()->view());
//       app.renderer()->endFrame();
//     } else {
//       // Filament wants to skip rendering this frame.
//     }
//
//     // Maybe sleep or block here until if you wish to limit frame rate.
//   }
//
//   // Your teardown code here.
//
//   app = {};
//

#ifndef FILAMENT_GLFW_IMGUI_H_
#define FILAMENT_GLFW_IMGUI_H_

#include <GLFW/glfw3.h>
#include <filament/Engine.h>
#include <filament/Material.h>
#include <filament/Renderer.h>
#include <filament/SwapChain.h>
#include <imgui/imgui.h>

#include <cstdint>
#include <ostream>

#include "filament_glfw_imgui/filament_imgui.h"
#include "filament_glfw_imgui/glfw_input.h"
#include "filament_glfw_imgui/glfw_input_imgui.h"
#include "filament_native/filament_native.h"

namespace filament_glfw_imgui {

class App {
 public:
  App() = default;

  // Inputs:
  //   window: must outlive this class. If null, Init() return 'false'.
  //   imgui_filamat: must outlive Init(). If null, Init() returns 'false'.
  //   log: must outlive this class, or if null, logging is silenced.
  App(GLFWwindow* window, const uint8_t* imgui_filamat,
      size_t imgui_filamat_size, std::ostream* log = &std::cout);

  App(const App&) = delete;
  App& operator=(const App&) = delete;

  App(App&& other);
  App& operator=(App&& other);

  // Params passed via the constructor.
  GLFWwindow* window() const { return window_; }
  const uint8_t* imgui_filamat() const { return imgui_filamat_; }
  size_t imgui_filamat_size() const { return imgui_filamat_size_; }
  std::ostream* log() const { return log_; }

  // Fields created by Init() and destroyed in ~App().
  filament::Engine* engine() const { return engine_; }
  filament::SwapChain* swap_chain() const { return swap_chain_; }
  filament::Renderer* renderer() const { return renderer_; }
  ImGuiContext* ui_context() const { return ui_context_; }
  filament::Material* ui_mat() const { return ui_mat_; }
  filament_imgui::Ui* ui() const { return ui_.get(); }
  glfw_input::WithImGui* input() const { return input_.get(); }

  // Initializes all derived fields.
  // Returns:
  //  'true' on succes.
  //  'false' and logs an error if 'log' was provided in the constructor.
  bool Init();

  // Returns 'true' if the app's mainloop should continue.
  bool Run();

  // Polls for input events. (May be nullptr if 'window' was null in ctor.)
  // TODO(ambrus): implement a version for glfwWaitEvents(...).
  glfw_input::State* PollEvents();

  // Updates the ImGui font atlas and calls ImGui::NewFrame().
  //  - Fonts may NOT be added between Begin/End-UiFrame().
  void BeginUiFrame();

  // Calls ImGui::Render and updates the Filament ui()->view().
  void EndUiFrame();

  // Calls renderer->beginFrame(...) on the swap chain.
  bool BeginRender();

  // Destroys everything created in Init().
  ~App();

 private:
  GLFWwindow* window_ = nullptr;            // Not owned.
  const uint8_t* imgui_filamat_ = nullptr;  // Not owned.
  size_t imgui_filamat_size_ = 0;
  std::ostream* log_ = nullptr;  // Not owned.

  filament::Engine* engine_ = nullptr;
  filament::SwapChain* swap_chain_ = nullptr;
  filament::Renderer* renderer_ = nullptr;

  ImGuiContext* ui_context_ = nullptr;
  filament::Material* ui_mat_ = nullptr;

  // NOTE(ambrus): I'd like these to be by-value fields, but it messes up the
  // "const-correctness" of the accessors.
  std::unique_ptr<filament_imgui::Ui> ui_ = nullptr;
  std::unique_ptr<glfw_input::WithImGui> input_ = nullptr;
};

}  // namespace filament_glfw_imgui

#include "filament_glfw_imgui/filament_glfw_imgui_impl.h"

#endif  // FILAMENT_GLFW_IMGUI_H_
