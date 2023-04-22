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
// A conversion for ImGui draw commands to Filament.
//
// See filament_glfw_imgui.h for an integrated, working example.
//
// NOTE: this file isn't part of the google/filament release. It is part of
// ambrusc/filament-glfw-imgui.
//
// Recommended Usage:
//
//   auto material = // Load filament_imgui.filamat
//   auto ui = filament_imgui::Ui(engine, material);
//
//   // Add fonts.
//   filamat_imgui::AddFont(...);
//
//   while (...) {  // Your main loop.
//
//     // Optionally add more fonts anytime before ImGui::NewFrame().
//     filamat_imgui::AddFont(...);
//     if (!ImGui::GetIO().Fonts->IsBuilt) {
//       ui.RebuiltFontAtlas(*ImGui::GetIO().Fonts);
//     }
//
//     ImGui::NewFrame();
//     // Your ImGui calls here.
//
//     ImGui::Render();
//     ImGuiIO& io = ImGui::GetIO();
//     ImGui::GetDrawData()->ScaleClipRects(io.DisplayFramebufferScale);
//     ui.UpdateView(*ImGui::GetDrawData(), io);
//
//     if (renderer->beginFrame(swap_chain)) {
//       // Your render calls.

//       renderer->render(ui.view());   // Show ImGui UI.
//       renderer->endFrame();
//     }
//   }
//

#ifndef FILAMENT_IMGUI_H_
#define FILAMENT_IMGUI_H_

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <imgui/imgui.h>
#include <utils/Entity.h>
#include <utils/Path.h>

#include <vector>

namespace filament_imgui {

// Adds a named font to ImFontAtlas in a single call.
template <size_t name_size>
ImFont *AddFont(const char (&name)[name_size], void *data, size_t data_size,
                float size_px, bool free_when_done, ImFontAtlas &atlas);

// Adds a named font to ImFontAtlas in a single call.
ImFont *AddFont(const char *name, size_t name_size, void *data,
                size_t data_size, float size_px, bool free_when_done,
                ImFontAtlas &atlas);

// Manages Filament state WITHOUT ever calling global ImGui functions.
//  - What you pass in is what's used, nothing more.
class Ui {
 public:
  Ui() = default;
  // Provide a valid engine and material for the UI to use.
  //   engine=nullptr => all UI components will be nullptr.
  //   material=nullptr => memory corruption.
  Ui(filament::Engine *engine, filament::Material *material);

  ~Ui();

  Ui(const Ui &) = delete;
  Ui &operator=(const Ui &) = delete;

  Ui(Ui &&);
  Ui &operator=(Ui &&);

  // Extracts the font atlas texture from ImFontAtlas.
  //  - Must call before ImGui::NewFrame() if !fonts.Built().
  //  - Some state changes are cached in ImFontAtlas, so we take it as &.
  void RebuildFontAtlas(ImFontAtlas &fonts);

  // Updates view() to with the latest UI state for rendering.
  //  - Must call after ImGui::Render() and before rendering view().
  //  - May wait on an engine fence internally.
  void UpdateView(const ImDrawData &commands, const ImGuiIO &io);

  // Render this view after your other views.
  filament::View *view() const { return view_; }

 private:
  filament::Engine *engine_ = nullptr;      // Not owned.
  filament::Material *material_ = nullptr;  // Not owned.

  filament::View *view_ = nullptr;
  filament::Scene *scene_ = nullptr;
  filament::Camera *camera_ = nullptr;

  filament::Texture *font_atlas_ = nullptr;
  filament::VertexBuffer *vertex_buffer_ = nullptr;
  filament::IndexBuffer *index_buffer_ = nullptr;
  std::vector<filament::MaterialInstance *> material_instances_;

  utils::Entity ui_entity_ = {};
  utils::Entity camera_entity_ = {};

  // Cached between frames.
  std::vector<ImDrawVert> vertex_data_;
  std::vector<ImDrawIdx> index_data_;
};

}  // namespace filament_imgui

#include "filament_glfw_imgui/filament_imgui_impl.h"

#endif /* FILAMENT_IMGUI_H_ */
