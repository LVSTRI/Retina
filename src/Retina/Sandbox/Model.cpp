#include <Retina/Sandbox/Model.hpp>

#include <glm/gtc/type_ptr.hpp>

#include <numeric>

namespace Retina::Sandbox {
  CModel::~CModel() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_data) {
      RETINA_CORE_INFO("Freeing glTF data: {}", static_cast<const void*>(_data));
      cgltf_free(_data);
    }
  }

  CModel::CModel(CModel&& other) noexcept
    : _data(std::exchange(other._data, nullptr)),
      _files(std::exchange(other._files, {})),
      _meshes(std::exchange(other._meshes, {})),
      _primitives(std::exchange(other._primitives, {})),
      _nodes(std::exchange(other._nodes, {}))
  {
    RETINA_PROFILE_SCOPED();
  }

  auto CModel::operator =(CModel&& other) noexcept -> CModel& {
    RETINA_PROFILE_SCOPED();
    if (this == &other) {
      return *this;
    }
    return Core::Reconstruct(*this, std::move(other));
  }

  auto CModel::Make(const std::filesystem::path& path) noexcept -> std::expected<CModel, EError> {
    RETINA_PROFILE_SCOPED();
    auto self = CModel();
    if (!std::filesystem::exists(path)) {
      return std::unexpected(EError::E_FILE_NOT_FOUND);
    }

    auto* gltf = static_cast<cgltf_data*>(nullptr);
    {
      auto options = cgltf_options();
      options.file = {
        .read = [](
          const struct cgltf_memory_options*,
          const struct cgltf_file_options* options,
          const char* path,
          cgltf_size* size,
          void** data
        ) -> cgltf_result {
          auto& self = *static_cast<CModel*>(options->user_data);
          auto file = mio::mmap_sink(path);
          if (!file.is_open()) {
            return cgltf_result_io_error;
          }
          *size = file.size();
          *data = file.data();
          self._files.emplace_back(std::move(file));
          return cgltf_result_success;
        },
        .release = [](
          const struct cgltf_memory_options*,
          const struct cgltf_file_options* options,
          void* data
        ) -> void {},
        .user_data = &self
      };
      {
        auto result = cgltf_parse_file(&options, path.generic_string().c_str(), &gltf);
        if (result != cgltf_result_success) {
          return std::unexpected(EError::E_FILE_PARSE_FAILURE);
        }
        result = cgltf_load_buffers(&options, gltf, path.generic_string().c_str());
        if (result != cgltf_result_success) {
          return std::unexpected(EError::E_BUFFER_LOAD_FAILURE);
        }
      }

      auto primitives = std::vector<SPrimitive>();
      auto meshes = std::vector<SMesh>();
      auto nodes = std::vector<SNode>();
      auto primitiveIndices = Core::FlatHashMap<const cgltf_primitive*, uint32>();
      for (auto i = 0_u32; i < gltf->meshes_count; ++i) {
        const auto& currentMesh = gltf->meshes[i];
        for (auto j = 0_u32; j < currentMesh.primitives_count; ++j) {
          const auto& currentPrimitive = currentMesh.primitives[j];

          auto primitive = SPrimitive();
          for (auto k = 0_u32; k < currentPrimitive.attributes_count; ++k) {
            const auto& attribute = currentPrimitive.attributes[k];
            const auto& accessor = *attribute.data;
            const auto& bufferView = *accessor.buffer_view;
            const auto& buffer = *bufferView.buffer;
            const auto& data = static_cast<const uint8*>(buffer.data) + bufferView.offset + accessor.offset;
            const auto& count = accessor.count;
            switch (attribute.type) {
              case cgltf_attribute_type_position: {
                primitive.Positions = std::span(
                  reinterpret_cast<const glm::vec3*>(data),
                  count
                );
                break;
              }

              case cgltf_attribute_type_normal: {
                primitive.Normals = std::span(
                  reinterpret_cast<const glm::vec3*>(data),
                  count
                );
                break;
              }


              case cgltf_attribute_type_texcoord: {
                primitive.Uvs = std::span(
                  reinterpret_cast<const glm::vec2*>(data),
                  count
                );
                break;
              }

              case cgltf_attribute_type_tangent: {
                primitive.Tangents = std::span(
                  reinterpret_cast<const glm::vec4*>(data),
                  count
                );
                break;
              }
            }

            if (primitive.Positions.empty()) {
              RETINA_CORE_WARN("Primitive has no positions, skipping");
              continue;
            }

            {
              auto& indices = *currentPrimitive.indices;
              const auto& bufferView = *indices.buffer_view;
              const auto& buffer = *bufferView.buffer;
              const auto& data = static_cast<const uint8*>(buffer.data) + bufferView.offset + indices.offset;
              const auto& count = indices.count;
              switch (indices.component_type) {
                case cgltf_component_type_r_8u: {
                  primitive.Indices = std::span(
                    reinterpret_cast<const uint8*>(data),
                    count
                  );
                  break;
                }

                case cgltf_component_type_r_16u: {
                  primitive.Indices = std::span(
                    reinterpret_cast<const uint16*>(data),
                    count
                  );
                  break;
                }

                case cgltf_component_type_r_32u: {
                  primitive.Indices = std::span(
                    reinterpret_cast<const uint32*>(data),
                    count
                  );
                  break;
                }
              }

              if (!count) {
                RETINA_CORE_WARN("Primitive has no indices, generating temporary indices");
                auto indices = std::vector<uint32>(primitive.Positions.size());
                std::iota(indices.begin(), indices.end(), 0);
                primitive.Indices = std::span(indices);
                continue;
              }
            }

            primitives.emplace_back(primitive);
            primitiveIndices.emplace(&currentPrimitive, primitives.size() - 1);
          }
        }

        auto mesh = SMesh();
        for (auto j = 0_u32; j < currentMesh.primitives_count; ++j) {
          const auto& currentPrimitive = currentMesh.primitives[j];
          mesh.Primitives.emplace_back(primitiveIndices[&currentPrimitive]);
        }
        meshes.emplace_back(std::move(mesh));
      }

      for (auto i = 0_u32; i < gltf->nodes_count; ++i) {
        const auto& currentNode = gltf->nodes[i];
        if (!currentNode.mesh) {
          RETINA_CORE_WARN("Node has no mesh, skipping");
          continue;
        }
        const auto meshIndex = cgltf_mesh_index(gltf, currentNode.mesh);
        auto transform = glm::mat4(1.0f);
        cgltf_node_transform_world(&currentNode, glm::value_ptr(transform));
        auto node = SNode();
        node.Mesh = meshIndex;
        node.Transform = transform;
        nodes.emplace_back(node);
      }

      RETINA_CORE_INFO("Loaded glTF file: {}", path.generic_string());
      RETINA_CORE_INFO("- Meshes: {}", meshes.size());
      RETINA_CORE_INFO("- Primitives: {}", primitives.size());
      RETINA_CORE_INFO("- Nodes: {}", nodes.size());
      RETINA_CORE_INFO("I/O Info:");
      RETINA_CORE_INFO("- Files: {}", self._files.size());

      self._meshes = std::move(meshes);
      self._primitives = std::move(primitives);
      self._nodes = std::move(nodes);
      self._data = gltf;
      return self;
    }
  }
}
