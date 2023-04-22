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
// Sample code for setting up and using the IBL prefiltering library provided
// alongside Filament.
//
// namespace "fs" stands for Filament Sample.
//

#ifndef FS_ENV_PREFILTER_H_
#define FS_ENV_PREFILTER_H_

#include <filament-iblprefilter/IBLPrefilterContext.h>
#include <filament/Engine.h>

#include <ostream>

namespace fs {

class Environment;

// Loader and filter for image-based lighting environments.
class EnvPrefilter {
 public:
  // Creates a bank of IBLPrefilters for this engine.
  //  - Set 'log' to nullptr to silence logging.
  explicit EnvPrefilter(filament::Engine& engine,
                        std::ostream* log = &std::cout);

  // Loads and filters an environment for image-based-lighting and reflections.
  bool LoadEquirect(const char* path, Environment& env);

  // Field accessors.
  filament::Engine& engine() { return engine_; }
  IBLPrefilterContext& context() { return context_; }
  IBLPrefilterContext::EquirectangularToCubemap& equirect_to_cube() {
    return equirect_to_cube_;
  }
  IBLPrefilterContext::SpecularFilter& specular_to_diffuse() {
    return specular_to_diffuse_;
  }

 private:
  filament::Engine& engine_;  // Not owned.
  std::ostream* log_;         // Not owned.

  IBLPrefilterContext context_;
  IBLPrefilterContext::EquirectangularToCubemap equirect_to_cube_;
  IBLPrefilterContext::SpecularFilter specular_to_diffuse_;
};

class Environment {
 public:
  // If 'engine' is non-null, dtor calls engine->destroy on other arguments.
  Environment() = default;
  Environment(filament::Engine* engine, filament::Texture* skybox_cube,
              filament::Skybox* skybox, filament::Texture* ibl_cube,
              filament::IndirectLight* ibl);

  Environment(const Environment&) = delete;
  Environment& operator=(const Environment&) = delete;

  Environment(Environment&& other);
  Environment& operator=(Environment&& other);

  ~Environment();

  filament::Engine* engine() const { return engine_; }
  filament::Texture* skybox_cube() const { return skybox_cube_; }
  filament::Skybox* skybox() const { return skybox_; }
  filament::Texture* ibl_cube() const { return ibl_cube_; }
  filament::IndirectLight* ibl() const { return ibl_; }

 private:
  filament::Engine* engine_ = nullptr;  // Not owned.
  filament::Texture* skybox_cube_ = nullptr;
  filament::Skybox* skybox_ = nullptr;
  filament::Texture* ibl_cube_ = nullptr;
  filament::IndirectLight* ibl_ = nullptr;
};

}  // namespace fs

#include "fs_env_prefilter_impl.h"

#endif  // FS_ENV_PREFILTER_H_
