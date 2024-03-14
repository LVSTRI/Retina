#pragma once

#include <Retina/Core/Core.hpp>

#include <Model.hpp>

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

    RETINA_NODISCARD static auto Make(const std::filesystem::path& path) noexcept -> CMeshletModel;

  private:
    std::optional<CModel> _model = {};

    std::vector<SMeshlet> _meshlets;
    std::vector<SMeshletInstance> _meshletInstances;
    std::vector<glm::mat4> _transforms;
    std::vector<glm::vec3> _positions;
    std::vector<SMeshletVertex> _vertices;
    std::vector<uint32> _indices;
    std::vector<uint8> _primitives;
  };
}
