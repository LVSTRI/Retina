#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Sandbox/Model.hpp>

#include <expected>

namespace Retina::Sandbox {
  struct SMeshlet {
    uint32 VertexOffset = 0;
    uint32 IndexOffset = 0;
    uint32 IndexCount = 0;
    uint32 PrimitiveOffset = 0;
    uint32 PrimitiveCount = 0;
  };

  struct SMeshletInstance {
    uint32 MeshletIndex = 0;
    uint32 TransformIndex = 0;
  };

  struct SMeshletVertex {
    glm::vec3 Normal = {};
    glm::vec2 Uv = {};
    glm::vec4 Tangent = {};
  };

  class CMeshletModel {
  public:
    CMeshletModel() noexcept = default;
    ~CMeshletModel() noexcept = default;
    RETINA_DELETE_COPY(CMeshletModel);
    RETINA_DEFAULT_MOVE(CMeshletModel);

    RETINA_NODISCARD static auto Make(const std::filesystem::path& path) noexcept -> std::expected<CMeshletModel, CModel::EError>;

    RETINA_NODISCARD auto GetMeshlets() const noexcept -> std::span<const SMeshlet>;
    RETINA_NODISCARD auto GetMeshletInstances() const noexcept -> std::span<const SMeshletInstance>;
    RETINA_NODISCARD auto GetTransforms() const noexcept -> std::span<const glm::mat4>;
    RETINA_NODISCARD auto GetPositions() const noexcept -> std::span<const glm::vec3>;
    RETINA_NODISCARD auto GetVertices() const noexcept -> std::span<const SMeshletVertex>;
    RETINA_NODISCARD auto GetIndices() const noexcept -> std::span<const uint32>;
    RETINA_NODISCARD auto GetPrimitives() const noexcept -> std::span<const uint8>;

  private:
    std::vector<SMeshlet> _meshlets;
    std::vector<SMeshletInstance> _meshletInstances;
    std::vector<glm::mat4> _transforms;
    std::vector<glm::vec3> _positions;
    std::vector<SMeshletVertex> _vertices;
    std::vector<uint32> _indices;
    std::vector<uint8> _primitives;
  };
}
