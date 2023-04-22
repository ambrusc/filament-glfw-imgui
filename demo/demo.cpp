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

#include <GLFW/glfw3.h>
#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/IndirectLight.h>
#include <filament/LightManager.h>
#include <filament/Material.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/Skybox.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>
#include <imgui/imgui.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <thread>

#include "filament_glfw_imgui/filament_glfw_imgui.h"
#include "fs_env_prefilter.h"
#include "fs_orbit_controller.h"
#include "fs_primitives.h"
#include "resources.h"

static constexpr char kEnvNames[][128] = {
    "environments/flower_road_2k.hdr",
    "environments/flower_road_no_sun_2k.hdr",
    "environments/graffiti_shelter_2k.hdr",
    "environments/lightroom_14b.hdr",
    "environments/noon_grass_2k.hdr",
    "environments/parking_garage_2k.hdr",
    "environments/pillars_2k.hdr",
    "environments/studio_small_02_2k.hdr",
    "environments/syferfontein_18d_clear_2k.hdr",
    "environments/the_sky_is_on_fire_2k.hdr",
    "environments/venetian_crossroads_2k.hdr",
};

class Demo {
 public:
  Demo() = default;
  Demo(filament::Engine* engine) : engine_(engine) {}

  Demo(const Demo&) = delete;
  Demo& operator=(const Demo&) = delete;

  Demo(Demo&& other) { *this = std::move(other); }
  Demo& operator=(Demo&& other) {
    std::swap(engine_, other.engine_);

    std::swap(camera_entity_, other.camera_entity_);
    std::swap(direct_light_, other.direct_light_);

    std::swap(view_, other.view_);
    std::swap(scene_, other.scene_);
    std::swap(camera_, other.camera_);

    std::swap(i_env_, other.i_env_);
    std::swap(env_prefilter_, other.env_prefilter_);
    std::swap(env_, other.env_);
    std::swap(visual_, other.visual_);
    std::swap(orbit_controller_, other.orbit_controller_);

    return *this;
  }

  void Init() {
    if (!engine_) return;

    using namespace filament;

    // Scene configuration.
    camera_entity_ = utils::EntityManager::get().create();
    camera_ = engine_->createCamera(camera_entity_);
    orbit_controller_.Update();
    orbit_controller_.ApplyTo(camera_);

    scene_ = engine_->createScene();

    view_ = engine_->createView();
    view_->setPostProcessingEnabled(false);
    view_->setCamera(camera_);
    view_->setScene(scene_);
    view_->setBlendMode(filament::BlendMode::OPAQUE);

    // Set up direct lighting.
    direct_light_ = utils::EntityManager::get().create();
    LightManager::Builder(LightManager::Type::SUN)
        .color(Color::toLinear<ACCURATE>(sRGBColor(0.98f, 0.92f, 0.89f)))
        .intensity(110000)
        .direction({0, 0, 1})
        .sunAngularRadius(1.9f)
        .castShadows(false)
        .build(*engine_, direct_light_);
    scene_->addEntity(direct_light_);

    // Set up image-based lighting.
    i_env_ = 3;
    env_prefilter_ = std::make_unique<fs::EnvPrefilter>(*engine_);
    if (env_prefilter_->LoadEquirect(kEnvNames[i_env_], env_)) {
      std::cout << "IBL " << env_.ibl() << std::endl;
      std::cout << "IBL " << env_.skybox() << std::endl;
      scene_->setIndirectLight(env_.ibl());
      scene_->setSkybox(env_.skybox());
    }

    // Add something to draw.
    visual_ = fs::VisualSphere(*engine_, RESOURCES_LIT_VERTEX_COLOR_DATA,
                               RESOURCES_LIT_VERTEX_COLOR_SIZE);
    scene_->addEntity(visual_.entity());

    // Load ImGui fonts.
    ImGuiIO& imgui_io = ImGui::GetIO();
    filament_imgui::AddFont("Roboto18", (void*)RESOURCES_ROBOTO_REGULAR_DATA,
                            RESOURCES_ROBOTO_REGULAR_SIZE, 18,
                            /*free_when_done=*/false, *imgui_io.Fonts);
    filament_imgui::AddFont("Inconsolata18",
                            (void*)RESOURCES_INCONSOLATA_REGULAR_DATA,
                            RESOURCES_INCONSOLATA_REGULAR_SIZE, 18,
                            /*free_when_done=*/false, *imgui_io.Fonts);
  }

  void ProcessInput(const glfw_input::State& input) {
    // Handle event-based inputs.
    int increment_env = 0;
    for (const glfw_input::Event& event : input.events) {
      switch (event.type) {
        case glfw_input::Event::kCursorPos:
          if (event.cursor_pos.buttons.HasGlfwButton(GLFW_MOUSE_BUTTON_LEFT)) {
            orbit_controller_.MousePan(
                {event.cursor_pos.xoffset, event.cursor_pos.yoffset});
          }
          break;
        case glfw_input::Event::kScroll:
          orbit_controller_.MouseDolly(event.scroll.yoffset);
          break;
        case glfw_input::Event::kKey:
          if (event.key.action == GLFW_PRESS) {
            switch (event.key.key) {
              case GLFW_KEY_O:
                increment_env = -1;
                break;
              case GLFW_KEY_P:
                increment_env = 1;
                break;
            }
          }
          break;
      }
    }

    // Load the next environment if requested.
    if (increment_env) {
      const int i_next_env =
          fs::Clamped(i_env_ + increment_env, 0,
                      sizeof(kEnvNames) / sizeof(kEnvNames[0]) - 1);
      if (i_next_env != i_env_) {
        i_env_ = i_next_env;
        std::cout << "i_env " << i_env_ << std::endl;
        if (env_prefilter_->LoadEquirect(kEnvNames[i_env_], env_)) {
          scene_->setSkybox(env_.skybox());
          scene_->setIndirectLight(env_.ibl());
        }
      }
    }

    {  // Handle state-based inputs.
      float pan_horiz = input.keys.Axis(GLFW_KEY_A, GLFW_KEY_D);
      float pan_vert = input.keys.Axis(GLFW_KEY_S, GLFW_KEY_W);
      if (pan_horiz || pan_vert) {
        orbit_controller_.NonmousePan({pan_horiz, pan_vert});
      }
      float dolly = input.keys.Axis(GLFW_KEY_E, GLFW_KEY_Q);
      if (dolly) {
        orbit_controller_.NonmouseDolly(dolly);
      }
    }
  }

  void UpdateUi() {
    ImGui::ShowDemoWindow();

    constexpr ImGuiWindowFlags overlay_flags =
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoInputs;

    const auto color = ImGui::GetStyle().Colors[ImGuiCol_WindowBg];
    ImGui::PushStyleColor(ImGuiCol_WindowBg,
                          ImVec4(color.x, color.y, color.z, 0.7f));

    {  // Show an FPS counter.
      ImGui::SetNextWindowPos(ImVec2(10, 10));
      ImGui::SetNextWindowSize(ImVec2(0, 0));
      ImGui::Begin("FPSCounter", nullptr, overlay_flags);
      ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
      ImGui::End();
    }

    {  // Show Controls.
      ImGui::SetNextWindowPos(ImVec2(10, 50));
      ImGui::SetNextWindowSize(ImVec2(0, 0));
      ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[1]);
      ImGui::Begin("Controls", nullptr, overlay_flags);
      ImGui::Text("  mouse drag - move camera");
      ImGui::Text("mouse scroll - zoom");
      ImGui::Text("     w,a,s,d - move camera");
      ImGui::Text("         q,e - zoom");
      ImGui::Text("         o,p - change env");
      ImGui::End();
      ImGui::PopFont();
    }
    ImGui::PopStyleColor();

    orbit_controller_.Update();
    orbit_controller_.ApplyTo(camera_);
  }

  void Render(filament::Renderer& renderer) {
    using namespace filament;

    const ImGuiIO& io = ImGui::GetIO();
    const uint32_t width_px = io.DisplaySize.x * io.DisplayFramebufferScale.x;
    const uint32_t height_px = io.DisplaySize.y * io.DisplayFramebufferScale.y;
    const float aspect = float(width_px) / height_px;

    camera_->setProjection(45.0, aspect, 0.3, 1000, Camera::Fov::VERTICAL);
    view_->setViewport({0, 0, width_px, height_px});

    renderer.render(view_);
  }

  ~Demo() {
    if (!engine_) return;

    orbit_controller_ = {};
    visual_ = {};
    env_ = {};
    env_prefilter_ = {};

    engine_->destroy(view_);
    engine_->destroy(scene_);

    engine_->destroy(direct_light_);
    engine_->destroyCameraComponent(camera_entity_);

    auto& entity_manager = utils::EntityManager::get();
    entity_manager.destroy(camera_entity_);
    entity_manager.destroy(direct_light_);
  }

 private:
  filament::Engine* engine_ = nullptr;  // Not owned.

  utils::Entity camera_entity_ = {};
  utils::Entity direct_light_ = {};

  filament::View* view_ = nullptr;
  filament::Scene* scene_ = nullptr;
  filament::Camera* camera_ = nullptr;

  int i_env_ = 0;
  std::unique_ptr<fs::EnvPrefilter> env_prefilter_;
  fs::Environment env_;
  fs::Visual visual_;
  fs::OrbitController orbit_controller_;
};

int main(int argc, char** argv) {
  // Initialize GLFW
  if (!glfwInit()) {
    std::cout << "Failed to init GLFW." << std::endl;
    return -1;
  }

  // Set the GLFW hint to disable the creation of a context
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  GLFWwindow* window =
      glfwCreateWindow(640, 480, "Filament Glfw ImGui", NULL, NULL);

  auto app = filament_glfw_imgui::App(window, RESOURCES_FILAMENT_IMGUI_DATA,
                                      RESOURCES_FILAMENT_IMGUI_SIZE);
  if (!app.Init()) return 1;  // App does logging by default.

  auto demo = Demo(app.engine());
  demo.Init();

  // Loop until the user closes the window
  bool render_skipped = false;
  while (app.Run()) {
    if (render_skipped) {
      // An imperfect but workable way to wait for the next VSYNC.
      // Filament's renderer->beginFrame() called by app.BeginRender() appears
      // to return 'false' if a frame has already been rendered for the next
      // screen refresh. If true, doing this introduces some input delay because
      // a fast-running app effectively processes inputs with a two frame lag
      // this way, but it could be worse.
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
    } else {
      const glfw_input::State& input = *app.PollEvents();
      demo.ProcessInput(input);
      app.BeginUiFrame();
      demo.UpdateUi();
      app.EndUiFrame();
    }

    if (app.BeginRender()) {
      render_skipped = false;
      demo.Render(*app.renderer());
      app.renderer()->render(app.ui()->view());
      app.renderer()->endFrame();
    } else {
      render_skipped = true;
    }
  }

  demo = {};
  app = {};

  glfwDestroyWindow(window);
  glfwTerminate();

  return 0;
}
