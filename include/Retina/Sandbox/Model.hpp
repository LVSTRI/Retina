#pragma once

#include <Retina/Core/Core.hpp>

#include <cgltf.h>
#include <glm/glm.hpp>
#include <mio/mmap.hpp>

#include <expected>
#include <filesystem>
#include <span>
#include <variant>
#include <vector>

namespace Retina::Sandbox {
  struct SPrimitive {
    std::span<const glm::vec3> Positions;
    std::span<const glm::vec3> Normals;
    std::span<const glm::vec2> Uvs;
    std::span<const glm::vec4> Tangents;
    std::variant<
      std::span<const uint8>,
      std::span<const uint16>,
      std::span<const uint32>
    > Indices;
    uint32 MaterialIndex = -1;
  };

  struct SMesh {
    std::vector<uint32> Primitives;
  };

  struct SNode {
    uint32 Mesh = 0;
    glm::mat4 Transform = {};
  };

  struct STexture {
    std::span<const uint8> Data;
    bool IsNormal = false;
  };

  struct SMaterial {
    glm::vec3 BaseColorFactor = {};
    uint32 BaseColorTexture = -1;
    uint32 NormalTexture = -1;
  };

  class CModel {
  public:
    enum class EError {
      E_FILE_NOT_FOUND,
      E_FILE_PARSE_FAILURE,
      E_BUFFER_LOAD_FAILURE,
    };

  public:
    CModel() noexcept = default;
    ~CModel() noexcept;
    RETINA_DELETE_COPY(CModel);
    RETINA_DECLARE_MOVE(CModel);

    RETINA_NODISCARD static auto Make(const std::filesystem::path& path) noexcept -> std::expected<CModel, EError>;

    RETINA_NODISCARD auto GetMeshes() const noexcept -> std::span<const SMesh>;
    RETINA_NODISCARD auto GetPrimitives() const noexcept -> std::span<const SPrimitive>;
    RETINA_NODISCARD auto GetNodes() const noexcept -> std::span<const SNode>;
    RETINA_NODISCARD auto GetTextures() const noexcept -> std::span<const STexture>;
    RETINA_NODISCARD auto GetMaterials() const noexcept -> std::span<const SMaterial>;

  private:
    cgltf_data* _data = nullptr;
    std::vector<mio::mmap_sink> _files;

    std::vector<SMesh> _meshes;
    std::vector<SPrimitive> _primitives;
    std::vector<SNode> _nodes;
    std::vector<STexture> _textures;
    std::vector<SMaterial> _materials;
  };
}
