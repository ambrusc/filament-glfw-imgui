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
// Allows Filament to access and configure the native swap-chain.
//
// See filament_glfw_imgui.h for an integrated, working example.
//
// NOTE: this file isn't part of the google/filament release. It is part of
// ambrusc/filament-glfw-imgui.
//

#ifndef FILAMENT_NATIVE_H_
#define FILAMENT_NATIVE_H_

extern "C" void* InitAndGetNativeSwapChain(void* window);

extern "C" void UpdateNativeSwapChainSize(void* window);

#endif  // FILAMENT_NATIVE_H_
