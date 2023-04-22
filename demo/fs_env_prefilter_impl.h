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

#ifndef FS_ENV_PREFILTER_IMPL_H_
#define FS_ENV_PREFILTER_IMPL_H_

#include <stb/stb_image.h>

namespace fs {

inline EnvPrefilter::EnvPrefilter(filament::Engine& engine, std::ostream* log)
    : engine_(engine),
      log_(log),
      context_(engine),
      equirect_to_cube_(context_),
      specular_to_diffuse_(context_) {}

inline bool EnvPrefilter::LoadEquirect(const char* path, Environment& env) {
  using namespace filament;

  int n, w, h;
  math::float3* const data = (math::float3*)stbi_loadf(path, &w, &h, &n, 3);
  const size_t size = w * h * sizeof(math::float3);
  if (data == nullptr || n != 3) {
    if (log_) {
      (*log_) << "Could not decode image " << std::endl;
    }
    return false;
  }

  if (log_) {
    (*log_) << "EnvPrefilter::LoadEquirect: " << path << " " << w << "," << h
            << " " << n << std::endl;
  }

  Texture* const equirect = Texture::Builder()
                                .width((uint32_t)w)
                                .height((uint32_t)h)
                                .levels(0xff)
                                .format(Texture::InternalFormat::R11F_G11F_B10F)
                                .sampler(Texture::Sampler::SAMPLER_2D)
                                .build(engine_);

  equirect->setImage(engine_, 0,
                     Texture::PixelBufferDescriptor(
                         data, size, Texture::Format::RGB, Texture::Type::FLOAT,
                         [](void* buffer, size_t size, void* user) {
                           stbi_image_free(buffer);
                         }));

  auto skybox_cube = equirect_to_cube_(equirect);
  engine_.destroy(equirect);
  auto skybox =
      Skybox::Builder().environment(skybox_cube).showSun(true).build(engine_);

  // Looks like this is ~1/3 the sun?
  constexpr float kIndirectLightIntensity = 30000.0f;
  auto ibl_cube = specular_to_diffuse_(skybox_cube);
  auto ibl = IndirectLight::Builder()
                 .reflections(ibl_cube)
                 .intensity(kIndirectLightIntensity)
                 .build(engine_);

  env = {&engine_, skybox_cube, skybox, ibl_cube, ibl};
  return true;
}

inline Environment::Environment(filament::Engine* engine,
                                filament::Texture* skybox_cube,
                                filament::Skybox* skybox,
                                filament::Texture* ibl_cube,
                                filament::IndirectLight* ibl)
    : engine_(engine),
      skybox_cube_(skybox_cube),
      skybox_(skybox),
      ibl_cube_(ibl_cube),
      ibl_(ibl) {}

inline Environment::Environment(Environment&& other) {
  *this = std::move(other);
}

inline Environment& Environment::operator=(Environment&& other) {
  std::swap(engine_, other.engine_);
  std::swap(skybox_cube_, other.skybox_cube_);
  std::swap(skybox_, other.skybox_);
  std::swap(ibl_cube_, other.ibl_cube_);
  std::swap(ibl_, other.ibl_);
  return *this;
}

inline Environment::~Environment() {
  if (engine_) {
    // engine_->destroy(nullptr) is okay.
    engine_->destroy(skybox_cube_);
    engine_->destroy(skybox_);
    engine_->destroy(ibl_cube_);
    engine_->destroy(ibl_);
  }
}

}  // namespace fs

#endif  // ENV_PREFILTER_IMPL_H_
