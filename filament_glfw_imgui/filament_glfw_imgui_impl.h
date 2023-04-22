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

#ifndef FILAMENT_GLFW_IMGUI_IMPL_H_
#define FILAMENT_GLFW_IMGUI_IMPL_H_

#include <imgui/backends/imgui_impl_glfw.h>

namespace filament_glfw_imgui {

inline App::App(GLFWwindow* window, const uint8_t* imgui_filamat,
                size_t imgui_filamat_size, std::ostream* log)
    : window_(window),
      imgui_filamat_(imgui_filamat),
      imgui_filamat_size_(imgui_filamat_size),
      log_(log) {}

inline App::App(App&& other) { *this = std::move(other); }

inline App& App::operator=(App&& other) {
  std::swap(window_, other.window_);
  std::swap(imgui_filamat_, other.imgui_filamat_);
  std::swap(imgui_filamat_size_, other.imgui_filamat_size_);
  std::swap(log_, other.log_);

  std::swap(engine_, other.engine_);
  std::swap(swap_chain_, other.swap_chain_);
  std::swap(renderer_, other.renderer_);

  std::swap(ui_context_, other.ui_context_);
  std::swap(ui_mat_, other.ui_mat_);

  std::swap(ui_, other.ui_);
  std::swap(input_, other.input_);

  return *this;
}

inline bool App::Init() {
  // Discourage calling Init() more than once.
  if (engine_) {
    if (log_) {
      (*log_) << "App::Init -> false: Init() should only be called once."
              << std::endl;
    }
    return false;
  }

  // Make sure we have a valid window.
  if (!window_) {
    if (log_) {
      (*log_) << "App::Init -> false: GLFWWindow is null." << std::endl;
    }
    return false;
  }

  // Extract the native swap chain from the GLFW window for Filament.
  void* native_swap_chain = InitAndGetNativeSwapChain(window_);
  if (!native_swap_chain) {
    if (log_) {
      (*log_) << "App::Init -> false: Can't create Filament "
                 "swap chain because native swap chain is null. Maybe this "
                 "platform combination is not implemented?"
              << std::endl;
    }
    return false;
  }

  // Finish intializing Filament.
  engine_ = filament::Engine::create();
  swap_chain_ = engine_->createSwapChain(native_swap_chain);
  renderer_ = engine_->createRenderer();

  // Initialize ImGui, as well as GLFW and Filament bindings.
  ui_context_ = ImGui::CreateContext();
  ImGui::SetCurrentContext(ui_context_);
  ImGui_ImplGlfw_InitForOther(window_, /*install_callbacks=*/false);
  ui_mat_ = filament::Material::Builder()
                .package(imgui_filamat_, imgui_filamat_size_)
                .build(*engine_);
  ui_ = std::make_unique<filament_imgui::Ui>(engine_, ui_mat_);

  input_ = std::make_unique<glfw_input::WithImGui>();
  GlfwAttachInputCallbacksAndSetWindowUserPointer(*input_, *window_);

  // TODO(ambrus): consider doing null-checking on all created pointers.

  return true;
}

inline bool App::Run() { return engine_ && !glfwWindowShouldClose(window_); }

// Polls for input events.
// TODO(ambrus): implement a version for glfwWaitEvents(...).
inline glfw_input::State* App::PollEvents() {
  if (!engine_) return nullptr;

  input_->ClearEvents();
  glfwPollEvents();

  // TODO(ambrus): check for resize instead of updating every frame.
  UpdateNativeSwapChainSize(window_);

  return &input_->state();
}

inline void App::BeginUiFrame() {
  if (!engine_) return;
  ImGuiIO& io = ImGui::GetIO();
  if (!io.Fonts->IsBuilt()) ui_->RebuildFontAtlas(*io.Fonts);
  ImGui_ImplGlfw_NewFrame();  // Updates io.DeltaTime and display size.
  ImGui::NewFrame();
}

inline void App::EndUiFrame() {
  if (!engine_) return;
  ImGuiIO& io = ImGui::GetIO();
  ImGui::Render();
  ImGui::GetDrawData()->ScaleClipRects(io.DisplayFramebufferScale);
  ui_->UpdateView(*ImGui::GetDrawData(), io);
}

inline bool App::BeginRender() {
  if (!renderer_) return false;
  return renderer_->beginFrame(swap_chain_);
}

inline App::~App() {
  if (!engine_) return;

  ImGui_ImplGlfw_Shutdown();
  input_ = {};
  ui_ = {};
  ImGui::DestroyContext(ui_context_);

  engine_->destroy(ui_mat_);
  engine_->destroy(renderer_);
  engine_->destroy(swap_chain_);

  filament::Engine::destroy(engine_);
}

}  // namespace filament_glfw_imgui

#endif  // FILAMENT_GLFW_IMGUI_IMPL_H_
