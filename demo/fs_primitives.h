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

#ifndef FS_PRIMITIVES_H_
#define FS_PRIMITIVES_H_

#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/RenderableManager.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

namespace fs {

class Visual {
 public:
  Visual() = default;

  Visual(filament::Engine* engine, filament::Material* material,
         filament::VertexBuffer* vertex_buf, filament::IndexBuffer* index_buf,
         utils::Entity& entity)
      : engine_(engine),
        material_(material),
        vertex_buf_(vertex_buf),
        index_buf_(index_buf),
        entity_(entity) {}

  Visual(const Visual&) = delete;
  Visual& operator=(const Visual&) = delete;

  Visual(Visual&& other) {
    std::swap(engine_, other.engine_);
    std::swap(material_, other.material_);
    std::swap(vertex_buf_, other.vertex_buf_);
    std::swap(index_buf_, other.index_buf_);
    std::swap(entity_, other.entity_);
  }
  Visual& operator=(Visual&& other) {
    std::swap(engine_, other.engine_);
    std::swap(material_, other.material_);
    std::swap(vertex_buf_, other.vertex_buf_);
    std::swap(index_buf_, other.index_buf_);
    std::swap(entity_, other.entity_);
    return *this;
  }
  ~Visual() {
    if (engine_ && material_) engine_->destroy(material_);
    if (engine_ && vertex_buf_) engine_->destroy(vertex_buf_);
    if (engine_ && index_buf_) engine_->destroy(index_buf_);
    if (engine_) engine_->destroy(entity_);
    utils::EntityManager::get().destroy(entity_);
  }

  filament::Engine* engine() const { return engine_; }
  filament::Material* material() const { return material_; }
  filament::VertexBuffer* vertex_buf() const { return vertex_buf_; }
  filament::IndexBuffer* index_buf() const { return index_buf_; }
  utils::Entity entity() const { return entity_; }

 private:
  filament::Engine* engine_ = nullptr;
  filament::Material* material_ = nullptr;
  filament::VertexBuffer* vertex_buf_ = nullptr;
  filament::IndexBuffer* index_buf_ = nullptr;
  utils::Entity entity_;
};

// Shader should be a compiled .filamat (e.g. from resources, or loaded from
// file).
inline Visual VisualSphere(filament::Engine& engine, const uint8_t* shader,
                           size_t shader_size) {
  using namespace filament;

  struct Vertex {
    math::float3 position;
    math::quatf tangents;
    uint32_t color;
  };

  const int n_rows = 32;
  const int n_cols = 64;

  const int n_verts = (n_rows - 1) * n_cols + 2;
  const int n_inds = 3 * (n_cols * (n_rows - 1) * 2);

  auto verts = std::vector<Vertex>();
  auto inds = std::vector<uint16_t>();

  verts.reserve(n_verts);
  inds.reserve(n_inds);

  const float d_ph = float(M_PI) / n_rows;
  const float d_th = float(2 * M_PI) / n_cols;

  const float ph = d_ph;
  const float z = std::cos(ph);
  const float zr = std::sin(ph);

  const auto rot_ph = math::quatf::fromAxisAngle(math::float3(0, 1, 0), ph);

  verts.push_back({{0, 0, 1},
                   math::quatf::fromAxisAngle(math::float3(1, 0, 0), 0),
                   0xffff0000u});
  const size_t v_top = 0;
  const size_t v_base = v_top + 1;
  for (int i = 0; i < n_cols; ++i) {
    const size_t v_cur = v_base + i;
    const size_t v_next = v_base + (i + 1) % n_cols;
    inds.push_back(v_top);
    inds.push_back(v_cur);
    inds.push_back(v_next);

    const float th = i * d_th;
    const float x = zr * std::cos(th);
    const float y = zr * std::sin(th);
    const math::float3 dir(x, y, z);
    const auto rot_th = math::quatf::fromAxisAngle(math::float3(0, 0, 1), th);
    const auto tangents = rot_th * rot_ph;
    const auto perp = normalize(cross(dir, math::float3(0, 0, 1)));
    const uint32_t color = 0xffff0000u | (uint32_t(255 * ph / M_PI) << 8);
    verts.push_back({dir, tangents, color});
  }

  for (int i_row = 1; i_row < n_rows - 1; ++i_row) {
    const float ph = (i_row + 1) * d_ph;
    const float z = std::cos(ph);
    const float zr = std::sin(ph);

    const auto rot_ph = math::quatf::fromAxisAngle(math::float3(0, 1, 0), ph);

    const size_t v_base = verts.size();
    for (int i = 0; i < n_cols; ++i) {
      const size_t v_cur = v_base + i;
      const size_t v_next = v_base + (i + 1) % n_cols;
      const size_t v_a = v_cur - n_cols;
      const size_t v_b = v_next - n_cols;
      inds.push_back(v_a);
      inds.push_back(v_cur);
      inds.push_back(v_next);
      inds.push_back(v_a);
      inds.push_back(v_next);
      inds.push_back(v_b);

      const float th = i * d_th;
      const float x = zr * std::cos(th);
      const float y = zr * std::sin(th);
      const math::float3 dir(x, y, z);
      const auto rot_th = math::quatf::fromAxisAngle(math::float3(0, 0, 1), th);
      const auto tangents = rot_th * rot_ph;
      const auto perp = normalize(cross(dir, math::float3(0, 0, 1)));
      const uint32_t color = 0xffff0000u | (uint32_t(255 * ph / M_PI) << 8);
      // verts.push_back({dir, math::quatf::fromAxisAngle(perp, ph), color});
      verts.push_back({dir, tangents, color});
    }
  }

  {
    const size_t v_bot = verts.size();
    verts.push_back({{0, 0, -1},
                     math::quatf::fromAxisAngle(math::float3(1, 0, 0), M_PI),
                     0xffffff00u});

    const size_t v_base = v_bot - n_cols;
    for (int i = 0; i < n_cols; ++i) {
      const size_t v_cur = v_base + i;
      const size_t v_next = v_base + (i + 1) % n_cols;
      inds.push_back(v_next);
      inds.push_back(v_cur);
      inds.push_back(v_bot);
    }
  }

  const size_t verts_data_size = verts.size() * sizeof(Vertex);
  const size_t inds_data_size = inds.size() * sizeof(uint16_t);

  void* verts_data = malloc(verts_data_size);
  void* inds_data = malloc(inds_data_size);

  // Filament wants to own the data during async upload to the GPU, and there's
  // no simple way to give it ownership of the C++ vectors. For a quick and
  // dirty implementation, we can deal with the extra copy."
  std::memcpy(verts_data, verts.data(), verts_data_size);
  std::memcpy(inds_data, inds.data(), inds_data_size);

  static const auto FreeCallback = [](void* buffer, size_t size,
                                      void* user) -> void { free(buffer); };

  auto vb =
      VertexBuffer::Builder()
          .vertexCount(verts.size())
          .bufferCount(1)
          .attribute(VertexAttribute::POSITION, 0,
                     VertexBuffer::AttributeType::FLOAT3, 0, sizeof(Vertex))
          .attribute(VertexAttribute::TANGENTS, 0,
                     VertexBuffer::AttributeType::FLOAT4, 12, sizeof(Vertex))
          .attribute(VertexAttribute::COLOR, 0,
                     VertexBuffer::AttributeType::UBYTE4, 28, sizeof(Vertex))
          .normalized(VertexAttribute::COLOR)
          .build(engine);
  vb->setBufferAt(engine, 0,
                  VertexBuffer::BufferDescriptor(verts_data, verts_data_size,
                                                 FreeCallback));

  auto ib = IndexBuffer::Builder()
                .indexCount(inds.size())
                .bufferType(IndexBuffer::IndexType::USHORT)
                .build(engine);
  ib->setBuffer(engine, IndexBuffer::BufferDescriptor(inds_data, inds_data_size,
                                                      FreeCallback));

  auto mat = Material::Builder().package(shader, shader_size).build(engine);

  auto entity = utils::EntityManager::get().create();
  RenderableManager::Builder(1)
      .boundingBox({{-1, -1, -1}, {1, 1, 1}})
      .material(0, mat->getDefaultInstance())
      .geometry(0, RenderableManager::PrimitiveType::TRIANGLES, vb, ib, 0,
                inds.size())
      .culling(false)
      .receiveShadows(false)
      .castShadows(false)
      .build(engine, entity);

  return {&engine, mat, vb, ib, entity};
}

}  // namespace fs

#endif  // FS_PRIMTIIVES_H_
