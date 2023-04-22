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

#ifndef FILAMENT_IMGUI_IMPL_H_
#define FILAMENT_IMGUI_IMPL_H_

#include <filament/Fence.h>
#include <filament/RenderableManager.h>
#include <filament/TextureSampler.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>

#include <cstdlib>
#include <iostream>
#include <utility>
#include <vector>

namespace filament_imgui {

template <size_t name_size>
ImFont *AddFont(const char (&name)[name_size], void *data, size_t data_size,
                float size_px, bool free_when_done, ImFontAtlas &atlas) {
  return AddFont(name, name_size, data, data_size, size_px, free_when_done,
                 atlas);
}

inline ImFont *AddFont(const char *name, size_t name_size, void *data,
                       size_t data_size, float size_px, bool free_when_done,
                       ImFontAtlas &atlas) {
  ImFontConfig font_cfg = {};
  font_cfg.FontDataOwnedByAtlas = free_when_done;
  std::memcpy(font_cfg.Name, name, std::min(name_size, sizeof(font_cfg.Name)));
  font_cfg.Name[sizeof(font_cfg.Name) - 1] = '\0';  // Just in case...
  return atlas.AddFontFromMemoryTTF(data, data_size, size_px, &font_cfg);
}

inline filament::VertexBuffer *CreateVertexBuffer(filament::Engine &engine,
                                                  size_t vertex_count) {
  using namespace filament;
  return filament::VertexBuffer::Builder()
      .vertexCount(vertex_count)
      .bufferCount(1)
      .attribute(VertexAttribute::POSITION, 0,
                 VertexBuffer::AttributeType::FLOAT2, 0, sizeof(ImDrawVert))
      .attribute(VertexAttribute::UV0, 0, VertexBuffer::AttributeType::FLOAT2,
                 sizeof(filament::math::float2), sizeof(ImDrawVert))
      .attribute(VertexAttribute::COLOR, 0, VertexBuffer::AttributeType::UBYTE4,
                 2 * sizeof(filament::math::float2), sizeof(ImDrawVert))
      .normalized(VertexAttribute::COLOR)
      .build(engine);
}

inline filament::IndexBuffer *CreateIndexBuffer(filament::Engine &engine,
                                                size_t index_count) {
  using namespace filament;
  return IndexBuffer::Builder()
      .indexCount(index_count)
      .bufferType(IndexBuffer::IndexType::USHORT)
      .build(engine);
}

inline filament::Texture *CreateFontTexture(filament::Engine &engine,
                                            ImFontAtlas &fonts) {
  using namespace filament;

  unsigned char *temp_pixels = nullptr;
  int width = 0;
  int height = 0;
  int pixel_bytes = 0;
  // TODO(ambrus): consider using the 8bpp version, as ImGui recommends.
  fonts.GetTexDataAsRGBA32(&temp_pixels, &width, &height, &pixel_bytes);

  // NOTE(ambrus): we live with this copy because we don't know when Filament
  // will be done uploading the texture. Another option would be to require the
  // caller to add a Filament engine fence before further calls to ImFontAtlas,
  // but that's starting to get very implicit and I'd rather accept this
  // malloc/copy than force someone to debug memory corruption or frame
  // stutters. We could add a custom allocator here if we wanted, but we'd
  // probably have to do thread synchronization as well.
  const size_t size = width * height * pixel_bytes;
  void *pixels = malloc(size);
  std::memcpy(pixels, temp_pixels, size);

  auto tex = Texture::Builder()
                 .width((uint32_t)width)
                 .height((uint32_t)height)
                 .levels((uint8_t)1)
                 .format(Texture::InternalFormat::RGBA8)
                 .sampler(Texture::Sampler::SAMPLER_2D)
                 .build(engine);
  tex->setImage(
      engine, 0,
      Texture::PixelBufferDescriptor(
          pixels, size, Texture::Format::RGBA, Texture::Type::UBYTE,
          [](void *buffer, size_t size, void *user) { free(buffer); }));

  return tex;
}

inline void SetScissor(ImVec4 clip_rect, int height_px,
                       filament::MaterialInstance &mat) {
  mat.setScissor(clip_rect.x, height_px - clip_rect.w,
                 (uint16_t)(clip_rect.z - clip_rect.x),
                 (uint16_t)(clip_rect.w - clip_rect.y));
}

inline Ui::Ui(filament::Engine *engine, filament::Material *material)
    : engine_(engine), material_(material) {
  if (engine_) {
    using namespace filament;

    auto &entity_manager = utils::EntityManager::get();

    view_ = engine_->createView();
    view_->setPostProcessingEnabled(false);
    view_->setBlendMode(View::BlendMode::TRANSLUCENT);
    view_->setShadowingEnabled(false);

    scene_ = engine_->createScene();

    camera_entity_ = entity_manager.create();
    camera_ = engine_->createCamera(camera_entity_);

    // font_atlas_ created in RebuildFontAtlas(...)
    // vertex_buffer_ created in UpdateView(...)
    // index_buffer_ created in UpdateView(...)
    // material_instances_ created in UpdateView(...)

    ui_entity_ = entity_manager.create();

    // Initialize relationships.
    view_->setCamera(camera_);
    view_->setScene(scene_);
    scene_->addEntity(ui_entity_);
  }
}

inline Ui::~Ui() {
  if (engine_) {
    // Engine can handle destroy(nullptr).
    engine_->destroy(scene_);
    engine_->destroy(ui_entity_);
    engine_->destroy(view_);
    engine_->destroyCameraComponent(camera_entity_);

    auto &entity_manager = utils::EntityManager::get();
    entity_manager.destroy(ui_entity_);
    entity_manager.destroy(camera_entity_);

    for (auto m : material_instances_) engine_->destroy(m);
    engine_->destroy(vertex_buffer_);
    engine_->destroy(index_buffer_);
    engine_->destroy(font_atlas_);
  }
}

inline Ui::Ui(Ui &&other) { *this = std::move(other); }

inline Ui &Ui::operator=(Ui &&other) {
  std::swap(engine_, other.engine_);
  std::swap(material_, other.material_);

  std::swap(view_, other.view_);
  std::swap(scene_, other.scene_);
  std::swap(camera_, other.camera_);

  std::swap(font_atlas_, other.font_atlas_);
  std::swap(vertex_buffer_, other.vertex_buffer_);
  std::swap(index_buffer_, other.index_buffer_);
  std::swap(material_instances_, other.material_instances_);

  std::swap(ui_entity_, other.ui_entity_);
  std::swap(camera_entity_, other.camera_entity_);

  std::swap(vertex_data_, other.vertex_data_);
  std::swap(index_data_, other.index_data_);

  return *this;
}

inline void Ui::RebuildFontAtlas(ImFontAtlas &fonts) {
  if (!engine_) return;

  // TODO(ambrus): is this fence necessary? Perhaps we have to wait for any
  // pending render operations before destroying textures that may be in use.
  filament::Fence::waitAndDestroy(engine_->createFence());
  engine_->destroy(font_atlas_);  // Ok to call w/nullptr.
  font_atlas_ = CreateFontTexture(*engine_, fonts);
  // We use nullptr as the sentinel for the main font atlas.
  fonts.SetTexID(nullptr);
}

inline void Ui::UpdateView(const ImDrawData &commands, const ImGuiIO &io) {
  if (!engine_) return;

  using namespace filament;

  // Don't render if app is minimized.
  if (io.DisplaySize.x == 0 && io.DisplaySize.y == 0) return;
  const int width_px = io.DisplaySize.x * io.DisplayFramebufferScale.x;
  const int height_px = io.DisplaySize.y * io.DisplayFramebufferScale.y;

  // Update the camera and viewport.
  // TODO(ambrus): what do way pay for doing this every frame?
  view_->setViewport({0, 0, uint32_t(width_px), uint32_t(height_px)});
  camera_->setProjection(Camera::Projection::ORTHO, 0.0,
                         double(io.DisplaySize.x), double(io.DisplaySize.y),
                         0.0, 0.0, 1.0);

  // TODO(ambrus): capture some stats, and maybe 16-bit index overflow, and
  // expose it as fields on the class.
  //
  // std::cout << "Commands " << commands.CmdListsCount << std::endl;
  // std::cout << "Total vertex count: " << commands.TotalVtxCount << std::endl;
  // std::cout << "Total index count: " << commands.TotalIdxCount << std::endl;

  // We rebuild the renderable part of the entity each frame (or draw nothing).
  engine_->getRenderableManager().destroy(ui_entity_);
  if (commands.CmdListsCount == 0) return;

  // Determine if we have any GPU-side resources to swap out.
  const bool rebuild_vertex_buffer =
      (vertex_buffer_ == nullptr ||
       vertex_buffer_->getVertexCount() < commands.TotalVtxCount);
  const bool rebuild_index_buffer =
      (index_buffer_ == nullptr ||
       index_buffer_->getIndexCount() < commands.TotalIdxCount);

  // TODO(ambrus): a better way of reporting errors.
  // Issue a warning if the texture atlas has been invalidated.
  if (!io.Fonts->IsBuilt()) {
    std::cout << "ImGuiIO->Fonts->IsBuilt() -> false. RebuildFontAtlas must be "
                 "called before ImGui::NewFrame() and after ImGui::Render() if "
                 "the ImGuiIO->Fonts API was used to add new fonts.";
  }

  if (rebuild_vertex_buffer || rebuild_index_buffer) {
    // TODO(ambrus): is this fence necessary? Perhaps we have to wait for any
    // pending render operations before destroying buffers that may be in use.
    Fence::waitAndDestroy(engine_->createFence());

    if (rebuild_vertex_buffer) {
      engine_->destroy(vertex_buffer_);  // nullptr ok.
      vertex_buffer_ = CreateVertexBuffer(*engine_, commands.TotalVtxCount);
      vertex_data_.resize(commands.TotalVtxCount);
    }
    if (rebuild_index_buffer) {
      engine_->destroy(index_buffer_);  // nullptr ok.
      index_buffer_ = CreateIndexBuffer(*engine_, commands.TotalIdxCount);
      index_data_.resize(commands.TotalIdxCount);
    }
  }

  // Count how many renderables we need.
  int num_renderables = 0;
  for (int i = 0; i < commands.CmdListsCount; ++i) {
    num_renderables += commands.CmdLists[i]->CmdBuffer.size();
  }
  auto renderable_builder = RenderableManager::Builder(num_renderables);
  renderable_builder.boundingBox({{0, 0, 0}, {10000, 10000, 10000}})
      .culling(false);

  // Extend material instances to cover the number of renderables.
  if (material_instances_.size() < num_renderables) {
    const int i_first = material_instances_.size();
    material_instances_.resize(num_renderables);
    for (int i = i_first; i < material_instances_.size(); ++i) {
      // TODO(ambrus): null check material_ (maybe in ctor?).
      material_instances_[i] = material_->createInstance();
    }
  }

  // Create renderables.
  int i_vert = 0;
  int i_ind = 0;
  int i_renderable = 0;
  for (int i = 0; i < commands.CmdListsCount; ++i) {
    const ImDrawList &draw_list = *commands.CmdLists[i];

    const int num_verts = draw_list.VtxBuffer.Size;
    const int num_inds = draw_list.IdxBuffer.Size;

    // Copy the vertex data into our snapshot.
    std::memcpy(vertex_data_.data() + i_vert, draw_list.VtxBuffer.Data,
                num_verts * sizeof(ImDrawVert));

    // Copy the index data into our snapshot.
    if (i == 0) {
      std::memcpy(index_data_.data() + i_ind, draw_list.IdxBuffer.Data,
                  num_inds * sizeof(ImDrawIdx));
    } else {
      // Filament doesn't support offseting into a vertex buffer, so we have to
      // rewrite indices.
      for (int i = 0; i < num_inds; ++i) {
        index_data_[i_ind + i] = draw_list.IdxBuffer.Data[i] + i_vert;
      }
    }

    // Create each renderable.
    for (const auto &cmd : draw_list.CmdBuffer) {
      // Some commands are user callbacks. ImGui API dictates we call them and
      // then continue.
      if (cmd.UserCallback) {
        cmd.UserCallback(&draw_list, &cmd);
        continue;
      }

      auto &mat_instance = *material_instances_[i_renderable];
      SetScissor(cmd.ClipRect, height_px, mat_instance);

      mat_instance.setParameter(
          "albedo",
          cmd.GetTexID() ? (const Texture *)cmd.GetTexID() : font_atlas_,
          TextureSampler(TextureSampler::MinFilter::LINEAR,
                         TextureSampler::MagFilter::LINEAR));

      renderable_builder
          .geometry(i_renderable, RenderableManager::PrimitiveType::TRIANGLES,
                    vertex_buffer_, index_buffer_, cmd.IdxOffset + i_ind,
                    cmd.ElemCount)
          .blendOrder(i_renderable, i_renderable)
          .material(i_renderable, &mat_instance);

      ++i_renderable;
    }

    i_vert += num_verts;
    i_ind += num_inds;
  }

  // Our UI entity is attached to the scene. Add UI renderables to it.
  renderable_builder.build(*engine_, ui_entity_);

  // Schedule async copy of data to the GPU.
  if (i_vert) {
    vertex_buffer_->setBufferAt(
        *engine_, /*buffer_index=*/0,
        VertexBuffer::BufferDescriptor(vertex_data_.data(),
                                       i_vert * sizeof(ImDrawVert)));
  }
  if (i_ind) {
    index_buffer_->setBuffer(
        *engine_, IndexBuffer::BufferDescriptor(index_data_.data(),
                                                i_ind * sizeof(ImDrawIdx)));
  }
}

}  // namespace filament_imgui

#endif  // IMGUI_FILAMENT_IMPL_H_
