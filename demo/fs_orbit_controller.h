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

#ifndef FS_ORBIT_CONTROLLER_H_
#define FS_ORBIT_CONTROLLER_H_

#include <filament/Camera.h>
#include <math/vec3.h>

#include <cmath>

namespace fs {

template <typename T, typename U, typename V>
T Clamped(const T& value, U lo, V hi) {
  return std::max<T>(lo, std::min<T>(hi, value));
}

template <typename T, typename U, typename V>
void Clamp(T& value, U lo, V hi) {
  value = Clamped(value, lo, hi);
}

struct OrbitController {
  float radius = 5;
  float theta = 0;
  float phi = 0;
  filament::math::float3 target = {0, 0, 0};
  filament::math::float3 up = {0, 1, 0};
  filament::math::float3 position = {0, 0, 0};  // Derived.

  // beyond about 0.48, filament's lookAt seems to jump to a bad state, probably
  // because the 'up' vector converges with the eye direction.
  float phi_min = -0.48 * M_PI;
  float phi_max = 0.48 * M_PI;

  float radius_min = 1.5;
  float radius_max = 150.0;

  float mouse_pan_gain = 0.01f;
  float nonmouse_pan_gain = 0.1f;

  float mouse_dolly_gain = -0.1f;
  float nonmouse_dolly_gain = 0.05f;

  void MousePan(filament::math::float2 delta) {
    theta -= mouse_pan_gain * delta.x;
    phi += mouse_pan_gain * delta.y;
    EnforcePanBounds();
  }

  void NonmousePan(filament::math::float2 delta) {
    theta += nonmouse_pan_gain * delta.x;
    phi += nonmouse_pan_gain * delta.y;
    EnforcePanBounds();
  }

  void EnforcePanBounds() {
    while (theta > M_PI) theta -= 2 * M_PI;
    while (theta < -M_PI) theta += 2 * M_PI;
    Clamp(phi, phi_min, phi_max);
  }

  void MouseDolly(float delta) {
    radius += radius * mouse_dolly_gain * delta;
    Clamp(radius, radius_min, radius_max);
  }

  void NonmouseDolly(float delta) {
    radius += radius * nonmouse_dolly_gain * delta;
    Clamp(radius, radius_min, radius_max);
  }

  void Update() {
    const float y = radius * std::sin(phi);
    const float yr = std::cos(phi);
    const float x = radius * yr * std::sin(theta);
    const float z = radius * yr * std::cos(theta);
    position = {x, y, z};
  }

  void ApplyTo(filament::Camera* cam) { cam->lookAt(position, target, up); }
};

}  // namespace fs

#endif  // FS_ORBIT_CONTROLLER_H_
