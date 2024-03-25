#include <Retina/Sandbox/MeshletModel.hpp>
#include <Retina/Sandbox/Logger.hpp>

#include <meshoptimizer.h>

#include <execution>

namespace Retina::Sandbox {
  namespace Details {
    struct SVertex {
      glm::vec3 Position = {};
      glm::vec3 Normal = {};
      glm::vec2 Uv = {};
      glm::vec4 Tangent = {};
    };

    struct SOptimizedVertexData {
      std::vector<SVertex> Vertices;
      std::vector<uint32> Indices;
    };

    struct SMeshletGenerationOutput {
      std::vector<meshopt_Meshlet> Meshlets;
      std::vector<uint32> Indices;
      std::vector<uint8> Primitives;
    };

    RETINA_NODISCARD RETINA_INLINE auto GenerateSmoothVertexNormals(
      std::span<const glm::vec3> positions,
      std::span<const uint32> indices
    ) noexcept -> std::vector<glm::vec3> {
      RETINA_PROFILE_SCOPED();
      auto normals = std::vector<glm::vec3>(positions.size());
      for (auto i = 0_u32; i < indices.size() / 3; ++i) {
        const auto i0 = indices[i * 3 + 0];
        const auto i1 = indices[i * 3 + 1];
        const auto i2 = indices[i * 3 + 2];

        const auto v0 = positions[i0];
        const auto v1 = positions[i1];
        const auto v2 = positions[i2];

        const auto normal = glm::cross(v0 - v1, v2 - v1);

        normals[i0] += normal;
        normals[i1] += normal;
        normals[i2] += normal;
      }

      std::transform(
        std::execution::par,
        normals.begin(),
        normals.end(),
        normals.begin(),
        [](const auto& normal) -> glm::vec3 {
          return glm::normalize(normal);
        }
      );

      return normals;
    }

    RETINA_NODISCARD RETINA_INLINE auto OptimizeVertexData(
      std::span<const SVertex> vertices,
      std::span<const uint32> indices
    ) noexcept -> SOptimizedVertexData {
      RETINA_PROFILE_SCOPED();
      auto optimizedVertices = std::vector<SVertex>();
      auto optimizedIndices = std::vector<uint32>();
      auto remap = std::vector<uint32>(vertices.size());

      const auto optimizedVertexCount = meshopt_generateVertexRemap(
        remap.data(),
        indices.data(),
        indices.size(),
        vertices.data(),
        vertices.size(),
        sizeof(SVertex)
      );

      optimizedVertices.resize(optimizedVertexCount);
      meshopt_remapVertexBuffer(
        optimizedVertices.data(),
        vertices.data(),
        vertices.size(),
        sizeof(SVertex),
        remap.data()
      );
      optimizedIndices.resize(indices.size());
      meshopt_remapIndexBuffer(
        optimizedIndices.data(),
        indices.data(),
        indices.size(),
        remap.data()
      );

      meshopt_optimizeVertexCache(
        optimizedIndices.data(),
        optimizedIndices.data(),
        optimizedIndices.size(),
        optimizedVertexCount
      );
      meshopt_optimizeOverdraw(
        optimizedIndices.data(),
        optimizedIndices.data(),
        optimizedIndices.size(),
        reinterpret_cast<const float32*>(optimizedVertices.data()),
        optimizedVertices.size(),
        sizeof(SVertex),
        1.05f
      );
      meshopt_optimizeVertexFetch(
        optimizedVertices.data(),
        optimizedIndices.data(),
        optimizedIndices.size(),
        optimizedVertices.data(),
        optimizedVertices.size(),
        sizeof(SVertex)
      );
      return {
        std::move(optimizedVertices),
        std::move(optimizedIndices)
      };
    }

    RETINA_NODISCARD RETINA_INLINE auto GenerateMeshlets(
      std::span<const SVertex> vertices,
      std::span<const uint32> indices
    ) noexcept -> SMeshletGenerationOutput {
      RETINA_PROFILE_SCOPED();
      constexpr static auto maxMeshletIndices = 64_u32;
      constexpr static auto maxMeshletPrimitives = 124_u32;
      constexpr static auto meshletConeWeight = 0.0_f32;

      const auto maxMeshlets = meshopt_buildMeshletsBound(indices.size(), maxMeshletIndices, maxMeshletPrimitives);
      auto meshlets = std::vector<meshopt_Meshlet>(maxMeshlets);
      auto meshletIndices = std::vector<uint32>(maxMeshlets * maxMeshletIndices);
      auto meshletPrimitives = std::vector<uint8>(maxMeshlets * maxMeshletPrimitives);

      const auto meshletCount = meshopt_buildMeshlets(
        meshlets.data(),
        meshletIndices.data(),
        meshletPrimitives.data(),
        indices.data(),
        indices.size(),
        reinterpret_cast<const float32*>(vertices.data()),
        vertices.size(),
        sizeof(SVertex),
        maxMeshletIndices,
        maxMeshletPrimitives,
        meshletConeWeight
      );

      const auto& lastMeshlet = meshlets[meshletCount - 1];
      meshlets.resize(meshletCount);
      meshletIndices.resize(lastMeshlet.vertex_offset + lastMeshlet.vertex_count);
      meshletPrimitives.resize(lastMeshlet.triangle_offset + ((lastMeshlet.triangle_count * 3 + 3) & ~3));

      return {
        std::move(meshlets),
        std::move(meshletIndices),
        std::move(meshletPrimitives)
      };
    }
  }

  auto CMeshletModel::Make(const std::filesystem::path& path) noexcept -> std::expected<CMeshletModel, CModel::EError> {
    RETINA_PROFILE_SCOPED();
    auto self = CMeshletModel();
    auto model = RETINA_EXPECT(CModel::Make(path));

    auto modelMeshlets = std::vector<SMeshlet>();
    auto modelPositions = std::vector<glm::vec3>();
    auto modelVertices = std::vector<SMeshletVertex>();
    auto modelIndices = std::vector<uint32>();
    auto modelPrimitives = std::vector<uint8>();

    const auto modelMeshPrimitives = model.GetPrimitives();
    auto meshletPrimitiveMapping = std::vector<std::pair<uint32, uint32>>();
    meshletPrimitiveMapping.reserve(modelMeshPrimitives.size());
    for (const auto primitive : modelMeshPrimitives) {
      const auto indices = std::visit(
        [](const auto& indices) -> std::vector<uint32> {
          auto result = std::vector<uint32>(indices.size());
          std::transform(
            std::execution::par,
            indices.begin(),
            indices.end(),
            result.begin(),
            [](const auto index) -> uint32 {
              return index;
            }
          );
          return result;
        },
        primitive.Indices
      );

      auto positions = std::vector<glm::vec3>(primitive.Positions.begin(), primitive.Positions.end());
      auto normals = std::vector<glm::vec3>(primitive.Normals.begin(), primitive.Normals.end());
      auto uvs = std::vector<glm::vec2>(primitive.Uvs.begin(), primitive.Uvs.end());
      auto tangents = std::vector<glm::vec4>(primitive.Tangents.begin(), primitive.Tangents.end());

      if (normals.empty()) {
        normals = Details::GenerateSmoothVertexNormals(positions, indices);
      }
      if (uvs.empty()) {
        uvs.resize(positions.size());
      }
      if (tangents.empty()) {
        tangents.resize(positions.size());
      }

      auto vertices = std::vector<Details::SVertex>();
      vertices.reserve(positions.size());
      for (auto i = 0_u32; i < positions.size(); ++i) {
        vertices.emplace_back(
          positions[i],
          normals[i],
          uvs[i],
          tangents[i]
        );
      }

      const auto [
        optimizedVertices,
        optimizedIndices
      ] = Details::OptimizeVertexData(vertices, indices);

      const auto [
        meshlets,
        meshletIndices,
        meshletPrimitives
      ] = Details::GenerateMeshlets(optimizedVertices, optimizedIndices);

      const auto currentMeshletOffset = static_cast<uint32>(modelMeshlets.size());
      const auto currentVertexOffset = static_cast<uint32>(modelVertices.size());
      const auto currentIndexOffset = static_cast<uint32>(modelIndices.size());
      const auto currentPrimitiveOffset = static_cast<uint32>(modelPrimitives.size());

      modelMeshlets.resize(modelMeshlets.size() + meshlets.size());
      std::transform(
        std::execution::par,
        meshlets.begin(),
        meshlets.end(),
        modelMeshlets.begin() + currentMeshletOffset,
        [&](const auto& meshlet) -> SMeshlet {
          return {
            currentVertexOffset,
            currentIndexOffset + meshlet.vertex_offset,
            meshlet.vertex_count,
            currentPrimitiveOffset + meshlet.triangle_offset,
            meshlet.triangle_count
          };
        }
      );
      
      modelPositions.resize(modelPositions.size() + optimizedVertices.size());
      std::transform(
        std::execution::par,
        optimizedVertices.begin(),
        optimizedVertices.end(),
        modelPositions.begin() + currentVertexOffset,
        [](const auto& vertex) -> glm::vec3 {
          return vertex.Position;
        }
      );
      
      modelVertices.resize(modelVertices.size() + optimizedVertices.size());
      std::transform(
        std::execution::par,
        optimizedVertices.begin(),
        optimizedVertices.end(),
        modelVertices.begin() + currentVertexOffset,
        [](const auto& vertex) -> SMeshletVertex {
          return {
            vertex.Normal,
            vertex.Uv,
            vertex.Tangent
          };
        }
      );
      
      modelIndices.resize(modelIndices.size() + meshletIndices.size());
      std::copy(
        std::execution::par,
        meshletIndices.begin(),
        meshletIndices.end(),
        modelIndices.begin() + currentIndexOffset
      );

      modelPrimitives.resize(modelPrimitives.size() + meshletPrimitives.size());
      std::copy(
        std::execution::par,
        meshletPrimitives.begin(),
        meshletPrimitives.end(),
        modelPrimitives.begin() + currentPrimitiveOffset
      );

      meshletPrimitiveMapping.emplace_back(currentMeshletOffset, currentMeshletOffset + meshlets.size());
    }

    auto modelMeshletInstances = std::vector<SMeshletInstance>();
    auto modelTransforms = std::vector<glm::mat4>();
    {
      const auto meshes = model.GetMeshes();
      for (const auto& [meshIndex, transform] : model.GetNodes()) {
        const auto& mesh = meshes[meshIndex];
        const auto transformIndex = static_cast<uint32>(modelTransforms.size());
        for (const auto& primitiveIndex : mesh.Primitives) {
          const auto& meshPrimitive = modelMeshPrimitives[primitiveIndex];
          const auto [begin, end] = meshletPrimitiveMapping[primitiveIndex];
          for (auto i = begin; i < end; ++i) {
            modelMeshletInstances.emplace_back(i, transformIndex, meshPrimitive.MaterialIndex);
          }
        }
        modelTransforms.emplace_back(transform);
      }
    }

    self._meshlets = std::move(modelMeshlets);
    self._meshletInstances = std::move(modelMeshletInstances);
    self._transforms = std::move(modelTransforms);
    self._positions = std::move(modelPositions);
    self._vertices = std::move(modelVertices);
    self._indices = std::move(modelIndices);
    self._primitives = std::move(modelPrimitives);
    self._model = std::move(model);
    return self;
  }

  auto CMeshletModel::GetMeshlets() const noexcept -> std::span<const SMeshlet> {
    RETINA_PROFILE_SCOPED();
    return _meshlets;
  }

  auto CMeshletModel::GetMeshletInstances() const noexcept -> std::span<const SMeshletInstance> {
    RETINA_PROFILE_SCOPED();
    return _meshletInstances;
  }

  auto CMeshletModel::GetTransforms() const noexcept -> std::span<const glm::mat4> {
    RETINA_PROFILE_SCOPED();
    return _transforms;
  }

  auto CMeshletModel::GetPositions() const noexcept -> std::span<const glm::vec3> {
    RETINA_PROFILE_SCOPED();
    return _positions;
  }

  auto CMeshletModel::GetVertices() const noexcept -> std::span<const SMeshletVertex> {
    RETINA_PROFILE_SCOPED();
    return _vertices;
  }

  auto CMeshletModel::GetIndices() const noexcept -> std::span<const uint32> {
    RETINA_PROFILE_SCOPED();
    return _indices;
  }

  auto CMeshletModel::GetPrimitives() const noexcept -> std::span<const uint8> {
    RETINA_PROFILE_SCOPED();
    return _primitives;
  }

  auto CMeshletModel::GetTextures() const noexcept -> std::span<const STexture> {
    RETINA_PROFILE_SCOPED();
    return _model.GetTextures();
  }

  auto CMeshletModel::GetMaterials() const noexcept -> std::span<const SMaterial> {
    RETINA_PROFILE_SCOPED();
    return _model.GetMaterials();
  }
}
