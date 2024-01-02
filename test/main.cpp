#include <Retina/Core/External/HashMap.hpp>
#include <Retina/Core/Core.hpp>
#include <Retina/Graphics/Graphics.hpp>
#include <Retina/Platform/Platform.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <cgltf.h>

#include <mio/mmap.hpp>

#include <ktx.h>

#include <filesystem>
#include <algorithm>
#include <ranges>
#include <span>

#define FRAMES_IN_FLIGHT 2

using namespace Retina::Types;
using namespace Retina::Literals;
namespace fs = std::filesystem;

struct SViewInfo {
    glm::mat4 InvProjection = {};
    glm::mat4 InvView = {};
    glm::mat4 InvProjView = {};
    glm::vec4 Position = {};
};

class CCamera {
public:
    using Self = CCamera;

    CCamera(const Retina::CInput& input) noexcept : _input(input) {}
    ~CCamera() noexcept = default;

    static auto Make(const Retina::CInput& input) noexcept -> Self {
        return { input };
    }

    RETINA_NODISCARD auto GetInput() const noexcept -> const Retina::CInput& {
        return _input;
    }

    RETINA_NODISCARD auto GetPosition() const noexcept -> const glm::vec3& {
        return _position;
    }

    RETINA_NODISCARD auto GetFront() const noexcept -> const glm::vec3& {
        return _front;
    }

    RETINA_NODISCARD auto GetUp() const noexcept -> const glm::vec3& {
        return _up;
    }

    RETINA_NODISCARD auto GetRight() const noexcept -> const glm::vec3& {
        return _right;
    }

    RETINA_NODISCARD auto GetYaw() const noexcept -> float32 {
        return _yaw;
    }

    RETINA_NODISCARD auto GetPitch() const noexcept -> float32 {
        return _pitch;
    }

    auto Update(float32 deltaTime) noexcept -> void {
        const auto& input = GetInput();
        const auto viewSensitivity = 150.0f * deltaTime;
        const auto movementSpeed = 10.0f * deltaTime;
        if (input.IsKeyPressed(Retina::EKeyboard::E_LEFT)) {
            _yaw -= viewSensitivity;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_RIGHT)) {
            _yaw += viewSensitivity;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_UP)) {
            _pitch += viewSensitivity;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_DOWN)) {
            _pitch -= viewSensitivity;
        }
        _pitch = glm::clamp(_pitch, -89.0f, 89.0f);
        const auto yawRadians = glm::radians(_yaw);
        const auto pitchRadians = glm::radians(_pitch);

        if (input.IsKeyPressed(Retina::EKeyboard::E_W)) {
            _position.x += glm::cos(yawRadians) * movementSpeed;
            _position.z += glm::sin(yawRadians) * movementSpeed;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_S)) {
            _position.x -= glm::cos(yawRadians) * movementSpeed;
            _position.z -= glm::sin(yawRadians) * movementSpeed;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_A)) {
            _position -= _right * movementSpeed;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_D)) {
            _position += _right * movementSpeed;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_SPACE)) {
            _position.y += movementSpeed;
        }
        if (input.IsKeyPressed(Retina::EKeyboard::E_LEFT_SHIFT)) {
            _position.y -= movementSpeed;
        }

        _front = glm::normalize(glm::vec3(
            glm::cos(yawRadians) * glm::cos(pitchRadians),
            glm::sin(pitchRadians),
            glm::sin(yawRadians) * glm::cos(pitchRadians)
        ));
        _right = glm::normalize(glm::cross(_front, glm::vec3(0.0f, 1.0f, 0.0f)));
        _up = glm::normalize(glm::cross(_right, _front));
    }

    auto GetViewMatrix() const noexcept -> glm::mat4 {
        return glm::lookAt(_position, _position + _front, _up);
    }

private:
    glm::vec3 _position = { 0.0f, 1.0f, 0.0f };
    glm::vec3 _front = {};
    glm::vec3 _up = {};
    glm::vec3 _right = {};
    float32 _yaw = 0.0f;
    float32 _pitch = 0.0f;

    std::reference_wrapper<const Retina::CInput> _input;
};

struct SObjectInfo {
    uint32 VertexOffset = 0;
    uint32 IndexOffset = 0;
    uint32 MaterialIndex = -1_u32;
};

struct SMaterial {
    glm::vec3 BaseColor = {};
    glm::vec3 EmissionColor = {};
    float32 EmissionStrength = 0.0f;

    uint32 BaseColorTexture = -1_u32;
    uint32 NormalTexture = -1_u32;
};

struct STexture {
    std::string_view Name = {};
    std::span<const uint8> Data;
    bool IsNormal = false;
};

class CModel {
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
        uint32 VertexOffset = 0;
        uint32 IndexOffset = 0;
    };

    struct SMesh {
        std::vector<SPrimitive> Primitives;
    };

    struct SNode {
        glm::mat4 Transform = {};
        uint64 MeshIndex = 0;
        std::vector<uint64> PrimitiveIndices;
    };

public:
    using Self = CModel;

    CModel() noexcept = default;
    ~CModel() noexcept = default;

    CModel(const Self&) noexcept = delete;
    auto operator =(const Self&) noexcept -> Self& = delete;
    CModel(Self&&) noexcept = default;
    auto operator =(Self&&) noexcept -> Self& = default;

    static auto Make(fs::path path) noexcept -> Self {
        auto model = Self();
        auto file = mio::mmap_source(path.string());
        auto options = cgltf_options();
        auto data = static_cast<cgltf_data*>(nullptr);
        cgltf_parse(&options, file.data(), file.size(), &data);
        cgltf_load_buffers(&options, data, path.string().c_str());

        auto sourceTextureCache = Retina::External::FastHashMap<const cgltf_texture*, uint32>();
        auto sourceTextures = std::vector<STexture>();
        sourceTextures.reserve(data->materials_count);
        auto sourceMaterials = std::vector<SMaterial>();
        sourceMaterials.reserve(data->materials_count);
        const auto emplaceMaterialTexture = [&](const cgltf_texture* texture, bool isNormal) -> uint32 {
            if (!texture) {
                return -1_u32;
            }
            if (sourceTextureCache.contains(texture)) {
                return sourceTextureCache[texture];
            }
            const auto* image = texture->basisu_image;
            if (!image) {
                return -1_u32;
            }
            const auto* data = static_cast<const uint8*>(image->buffer_view->buffer->data);
            const auto offset = image->buffer_view->offset;
            const auto size = image->buffer_view->size;
            auto textureData = STexture();
            textureData.Name = image->uri
                ? image->uri
                : image->name
                    ? image->name
                    : std::string_view();
            textureData.Data = std::span(data + offset, size);
            textureData.IsNormal = isNormal;
            sourceTextures.emplace_back(textureData);
            const auto& [iterator, _0] = sourceTextureCache.emplace(texture, sourceTextures.size() - 1);
            const auto& [key, value] = *iterator;
            return value;
        };
        for (auto i = 0_u32; i < data->materials_count; ++i) {
            const auto& currentMaterial = data->materials[i];
            const auto* baseColorTexture = currentMaterial.pbr_metallic_roughness.base_color_texture.texture;
            const auto* normalTexture = currentMaterial.normal_texture.texture;
            auto material = SMaterial();
            if (currentMaterial.has_pbr_metallic_roughness) {
                const auto& pbr = currentMaterial.pbr_metallic_roughness;
                material.BaseColor = glm::make_vec3(pbr.base_color_factor);
            }
            material.EmissionColor = glm::make_vec3(currentMaterial.emissive_factor);
            if (currentMaterial.has_emissive_strength) {
                material.EmissionStrength = currentMaterial.emissive_strength.emissive_strength;
            }
            material.BaseColorTexture = emplaceMaterialTexture(baseColorTexture, false);
            material.NormalTexture = emplaceMaterialTexture(normalTexture, true);
            sourceMaterials.emplace_back(material);
        }
        model._textures = std::move(sourceTextures);
        model._materials = std::move(sourceMaterials);

        auto primitiveCache = Retina::External::FastHashMap<const cgltf_primitive*, uint32>();
        auto currentVertexOffset = 0_u32;
        auto currentIndexOffset = 0_u32;
        auto currentPrimitiveIndex = 0_u32;
        for (auto i = 0_u32; i < data->meshes_count; ++i) {
            const auto& currentMesh = data->meshes[i];
            const auto& currentMeshPrimitives = currentMesh.primitives;
            auto mesh = SMesh();
            mesh.Primitives.reserve(currentMesh.primitives_count);
            for (auto j = 0_u32; j < currentMesh.primitives_count; ++j) {
                const auto& currentPrimitive = currentMeshPrimitives[j];
                const auto& currentPrimitiveAttributes = currentPrimitive.attributes;
                auto primitive = SPrimitive();
                primitive.VertexOffset = currentVertexOffset;
                primitive.IndexOffset = currentIndexOffset;
                for (auto k = 0_u32; k < currentPrimitive.attributes_count; ++k) {
                    const auto& currentAttribute = currentPrimitiveAttributes[k];
                    const auto& accessor = *currentAttribute.data;
                    const auto& bufferView = *accessor.buffer_view;
                    const auto& buffer = *bufferView.buffer;
                    const auto offset = accessor.offset + bufferView.offset;
                    const auto* dataPtr = static_cast<const uint8*>(buffer.data) + offset;

                    switch (currentAttribute.type) {
                        case cgltf_attribute_type_position: {
                            primitive.Positions = {
                                reinterpret_cast<const glm::vec3*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        case cgltf_attribute_type_normal: {
                            primitive.Normals = {
                                reinterpret_cast<const glm::vec3*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        case cgltf_attribute_type_texcoord: {
                            primitive.Uvs = {
                                reinterpret_cast<const glm::vec2*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        case cgltf_attribute_type_tangent: {
                            primitive.Tangents = {
                                reinterpret_cast<const glm::vec4*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        default: break;
                    }
                }
                {
                    const auto& accessor = *currentPrimitive.indices;
                    const auto& bufferView = *accessor.buffer_view;
                    const auto& buffer = *bufferView.buffer;
                    const auto offset = accessor.offset + bufferView.offset;
                    const auto* dataPtr = static_cast<const uint8*>(buffer.data) + offset;
                    switch (accessor.component_type) {
                        case cgltf_component_type_r_8u: {
                            primitive.Indices = std::span<const uint8> {
                                reinterpret_cast<const uint8*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        case cgltf_component_type_r_16u: {
                            primitive.Indices = std::span<const uint16> {
                                reinterpret_cast<const uint16*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        case cgltf_component_type_r_32u: {
                            primitive.Indices = std::span<const uint32> {
                                reinterpret_cast<const uint32*>(dataPtr),
                                accessor.count,
                            };
                        } break;

                        default: break;
                    }
                }
                if (!primitive.Positions.empty()) {
                    if (!primitiveCache.contains(&currentPrimitive)) {
                        primitiveCache.emplace(&currentPrimitive, currentPrimitiveIndex++);
                        mesh.Primitives.emplace_back(primitive);
                        currentVertexOffset += primitive.Positions.size();
                        currentIndexOffset += std::visit(
                            [](const auto& indices) noexcept {
                                return indices.size();
                            },
                            primitive.Indices
                        );
                    }
                }
            }
            model._meshes.emplace_back(std::move(mesh));
        }

        for (auto i = 0_u32; i < data->nodes_count; ++i) {
            const auto& currentNode = data->nodes[i];
            if (!currentNode.mesh) {
                continue;
            }
            auto node = SNode();
            node.MeshIndex = cgltf_mesh_index(data, currentNode.mesh);
            const auto& mesh = model._meshes[node.MeshIndex];
            for (auto j = 0_u32; j < mesh.Primitives.size(); ++j) {
                const auto& currentPrimitive = currentNode.mesh->primitives[j];
                if (!primitiveCache.contains(&currentPrimitive)) {
                    continue;
                }
                const auto materialIndex = cgltf_material_index(data, currentPrimitive.material);
                const auto& primitive = mesh.Primitives[j];
                node.PrimitiveIndices.emplace_back(primitiveCache[&currentPrimitive]);

                auto objectInfo = SObjectInfo();
                objectInfo.VertexOffset = primitive.VertexOffset;
                objectInfo.IndexOffset = primitive.IndexOffset;
                objectInfo.MaterialIndex = materialIndex;

                model._objectInfos.emplace_back(objectInfo);
            }
            cgltf_node_transform_world(&currentNode, glm::value_ptr(node.Transform));
            model._nodes.emplace_back(std::move(node));
        }

        model._data = data;
        model._file = std::move(file);
        return model;
    }

    auto GetMeshes() const noexcept -> std::span<const SMesh> {
        return _meshes;
    }

    auto GetNodes() const noexcept -> std::span<const SNode> {
        return _nodes;
    }

    auto GetObjectInfos() const noexcept -> std::span<const SObjectInfo> {
        return _objectInfos;
    }

    auto GetTextures() const noexcept -> std::span<const STexture> {
        return _textures;
    }

    auto GetMaterials() const noexcept -> std::span<const SMaterial> {
        return _materials;
    }

private:
    std::vector<SMesh> _meshes;
    std::vector<SNode> _nodes;
    std::vector<SObjectInfo> _objectInfos;
    std::vector<STexture> _textures;
    std::vector<SMaterial> _materials;

    cgltf_data* _data = nullptr;
    mio::mmap_source _file;
};

static auto MakeBlasesFromModel(
    const Retina::CDevice& device,
    const CModel& model
) noexcept -> std::vector<Retina::CArcPtr<Retina::CBottomLevelAccelerationStructure>> {
    const auto meshes = model.GetMeshes();
    auto blas = std::vector<Retina::CArcPtr<Retina::CBottomLevelAccelerationStructure>>();
    blas.reserve(meshes.size());
    for (auto currentPrimitiveIndex = 0_u32; const auto& mesh : meshes) {
        for (const auto& primitive : mesh.Primitives) {
            auto meshPositionBuffer = std::vector<glm::vec3>(
                primitive.Positions.begin(),
                primitive.Positions.end()
            );
            auto meshIndexBuffer = std::vector<uint32>();
            std::visit(
                [&](const auto& indices) noexcept {
                    meshIndexBuffer.reserve(indices.size());
                    std::ranges::transform(
                        indices,
                        std::back_inserter(meshIndexBuffer),
                        [&](const auto& index) noexcept {
                            return static_cast<uint32>(index);
                        }
                    );
                },
                primitive.Indices
            );
            auto vertexBuffer = Retina::CTypedBuffer<glm::vec3>::Upload(device, {
                .Name = "TemporaryVertexBuffer",
                .Data = meshPositionBuffer,
            });
            auto indexBuffer = Retina::CTypedBuffer<uint32>::Upload(device, {
                .Name = "TemporaryIndexBuffer",
                .Data = meshIndexBuffer,
            });
            blas.emplace_back(Retina::CBottomLevelAccelerationStructure::MakeCompact(device, {
                .Name = std::format("BLAS_Mesh{}", currentPrimitiveIndex),
                .Flags = Retina::EAccelerationStructureBuildFlag::E_PREFER_FAST_TRACE_KHR |
                         Retina::EAccelerationStructureBuildFlag::E_ALLOW_DATA_ACCESS_KHR,
                .GeometryInfos = {
                    {
                        .Range = {
                            .PrimitiveCount = static_cast<uint32>(meshIndexBuffer.size() / 3),
                        },
                        .Flags = Retina::EAccelerationStructureGeometryFlag::E_NO_DUPLICATE_ANY_HIT_INVOCATION_KHR,
                        .Data = Retina::SAccelerationStructureGeometryTrianglesData {
                            .PositionBuffer = vertexBuffer.Get(),
                            .IndexBuffer = indexBuffer.Get(),
                        },
                    }
                }
            }));
            currentPrimitiveIndex++;
        }
    }
    return blas;
}

static auto MakeTlasFromModel(
    const Retina::CDevice& device,
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable,
    std::span<const Retina::CArcPtr<Retina::CBottomLevelAccelerationStructure>> blases
) noexcept -> Retina::AccelerationStructureResource {
    const auto nodes = model.GetNodes();
    auto instances = std::vector<Retina::SAccelerationStructureGeometryInstance>();
    instances.reserve(nodes.size());
    for (auto currentNodeIndex = 0_u32; const auto& node : nodes) {
        for (const auto primitiveIndex : node.PrimitiveIndices) {
            instances.emplace_back(Retina::SAccelerationStructureGeometryInstance {
                .Transform = Retina::ToNativeTransformMatrix(node.Transform),
                .ObjectIndex = currentNodeIndex++,
                .AccelerationStructureAddress = blases[primitiveIndex]->GetAddress(),
            });
        }
    }
    auto instanceBuffer = Retina::CTypedBuffer<Retina::SAccelerationStructureGeometryInstance>::Upload(device, {
        .Name = "TLAS_InstanceBuffer",
        .Data = instances,
    });
    return shaderResourceTable.MakeAccelerationStructure({
        .Name = "TLAS_Main",
        .Flags = Retina::EAccelerationStructureBuildFlag::E_PREFER_FAST_TRACE_KHR |
                 Retina::EAccelerationStructureBuildFlag::E_ALLOW_DATA_ACCESS_KHR,
        .GeometryInfos = {
            {
                .Range = {
                    .PrimitiveCount = static_cast<uint32>(instances.size()),
                },
                .Flags = Retina::EAccelerationStructureGeometryFlag::E_NO_DUPLICATE_ANY_HIT_INVOCATION_KHR,
                .Data = Retina::SAccelerationStructureGeometryInstancesData {
                    .InstanceBuffer = instanceBuffer.Get(),
                },
            }
        }
    });
}

struct SVertex {
    glm::vec3 Normal = {};
    glm::vec2 Uv = {};
    glm::vec4 Tangent = {};
};
static auto MakeVertexBufferFromModel(
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable
) noexcept -> Retina::StorageBufferResource<SVertex> {
    const auto meshes = model.GetMeshes();
    auto vertexBuffer = std::vector<SVertex>();
    vertexBuffer.reserve(meshes.size());
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.Primitives) {
            for (auto i = 0_u32; i < primitive.Positions.size(); ++i) {
                auto normal = glm::vec3(0.0f);
                auto uv = glm::vec2(0.0f);
                auto tangent = glm::vec4(0.0f);
                if (!primitive.Normals.empty()) {
                    normal = primitive.Normals[i];
                }
                if (!primitive.Uvs.empty()) {
                    uv = primitive.Uvs[i];
                }
                if (!primitive.Tangents.empty()) {
                    tangent = primitive.Tangents[i];
                }
                vertexBuffer.emplace_back(SVertex {
                    .Normal = normal,
                    .Uv = uv,
                    .Tangent = tangent,
                });
            }
        }
    }
    return shaderResourceTable.MakeStorageBuffer<SVertex>({
        .Name = "VertexBuffer",
        .Data = vertexBuffer,
    });
}

static auto MakeIndexBufferFromModel(
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable
) noexcept -> Retina::StorageBufferResource<uint32> {
    const auto meshes = model.GetMeshes();
    auto indexBuffer = std::vector<uint32>();
    indexBuffer.reserve(meshes.size());
    for (const auto& mesh : meshes) {
        for (const auto& primitive : mesh.Primitives) {
            std::visit(
                [&](const auto& indices) noexcept {
                    indexBuffer.reserve(indexBuffer.capacity() + indices.size());
                    std::ranges::transform(
                        indices,
                        std::back_inserter(indexBuffer),
                        [&](const auto& index) noexcept {
                            return static_cast<uint32>(index);
                        }
                    );
                },
                primitive.Indices
            );
        }
    }
    return shaderResourceTable.MakeStorageBuffer<uint32>({
        .Name = "IndexBuffer",
        .Data = indexBuffer,
    });
}

static auto MakeObjectInfoBufferFromModel(
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable
) noexcept -> Retina::StorageBufferResource<SObjectInfo> {
    return shaderResourceTable.MakeStorageBuffer<SObjectInfo>({
        .Name = "ObjectInfoBuffer",
        .Data = model.GetObjectInfos(),
    });
}

static auto MakeMaterialBufferFromModel(
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable
) noexcept -> Retina::StorageBufferResource<SMaterial> {
    const auto materials = model.GetMaterials();
    return shaderResourceTable.MakeStorageBuffer<SMaterial>({
        .Name = "MaterialBuffer",
        .Data = materials,
    });
}

static auto MakeTexturesFromModel(
    const Retina::CDevice& device,
    const CModel& model,
    Retina::CShaderResourceTable& shaderResourceTable
) noexcept -> std::vector<Retina::SampledImageResource> {
    const auto textures = model.GetTextures();
    auto images = std::vector<Retina::SampledImageResource>();
    images.reserve(textures.size());
    for (const auto& texture : textures) {
        auto ktx = (ktxTexture2*)(nullptr);
        ktxTexture2_CreateFromMemory(
            texture.Data.data(),
            texture.Data.size(),
            KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
            &ktx
        );
        if (ktxTexture2_NeedsTranscoding(ktx)) {
            if (texture.IsNormal) {
                ktxTexture2_TranscodeBasis(ktx, KTX_TTF_BC5_RG, KTX_TF_HIGH_QUALITY);
            } else {
                ktxTexture2_TranscodeBasis(ktx, KTX_TTF_BC7_RGBA, KTX_TF_HIGH_QUALITY);
            }
        }
        const auto staging = Retina::CBuffer::Make(device, {
            .Name = "TextureStaging",
            .Heap = Retina::EMemoryProperty::E_DEVICE_MAPPABLE,
            .Capacity = ktx->dataSize,
        });
        staging->Write<uint8>({ ktx->pData, ktx->dataSize });
        auto image = shaderResourceTable.MakeSampledImage({
            .Name = std::format("Texture{}", images.size()),
            .Width = ktx->baseWidth,
            .Height = ktx->baseHeight,
            .Levels = ktx->numLevels,
            .Usage = Retina::EImageUsage::E_SAMPLED |
                     Retina::EImageUsage::E_TRANSFER_DST,
            .Format = static_cast<Retina::EResourceFormat>(ktx->vkFormat),
            .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
        });
        device.GetGraphicsQueue().Submit([&](Retina::CCommandBuffer& commands) {
            commands.ImageMemoryBarrier({
                .Image = *image,
                .SourceStage = Retina::EPipelineStage::E_NONE,
                .DestStage = Retina::EPipelineStage::E_TRANSFER,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
            });
            for (auto i = 0_u32; i < ktx->numLevels; ++i) {
                auto offset = 0_u64;
                ktxTexture_GetImageOffset(ktxTexture(ktx), i, 0, 0, &offset);
                commands.CopyBufferToImage(*staging, *image, {
                    .Offset = offset,
                    .Subresource = {
                        .BaseLevel = i,
                    }
                });
            }
            commands.ImageMemoryBarrier({
                .Image = *image,
                .SourceStage = Retina::EPipelineStage::E_TRANSFER,
                .DestStage = Retina::EPipelineStage::E_FRAGMENT_SHADER,
                .SourceAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_READ,
                .OldLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_SHADER_READ_ONLY_OPTIMAL,
            });
        });
    }
    return images;
}

int main() {
    auto window = Retina::CWindow::Make({
        .Title = "Retina Engine",
        .Width = 1280,
        .Height = 720,
        .Features = {
            .Resizable = true,
            .Decorated = true,
            .Focused = true,
        },
    });
    auto instance = Retina::CInstance::Make({
        .Features = {
            .Surface = true,
            .Debug = true,
        },
        .PlatformGetSurfaceExtensionsFunc = Retina::Platform::GetSurfaceExtensions,
    });
    auto device = Retina::CDevice::Make(*instance, {
        .Name = "MainDevice",
        .Extensions = {
            .Swapchain = true,
            .RayTracing = true,
        },
        .Features = {},
    });
    auto swapchain = Retina::CSwapchain::Make(*device, *window, {
        .Name = "MainSwapchain",
        .VSync = true,
        .MakeSurfaceFunc = Retina::Platform::MakeNativeSurface,
    });
    auto commandBuffers = Retina::CCommandBuffer::Make(device->GetGraphicsQueue(), FRAMES_IN_FLIGHT, {
        .Name = "MainCommandBuffer",
        .Primary = true,
        .CommandPoolInfo = Retina::Constant::DEFAULT_COMMAND_POOL_INFO
    });
    auto imageAvailableSemaphores = Retina::CBinarySemaphore::Make(*device, FRAMES_IN_FLIGHT, {
        .Name = "ImageAvailableSemaphore",
    });
    auto presentReadySemaphores = Retina::CBinarySemaphore::Make(*device, FRAMES_IN_FLIGHT, {
        .Name = "RenderFinishedSemaphore",
    });
    auto frameTimeline = Retina::CSyncHostDeviceTimeline::Make(*device, FRAMES_IN_FLIGHT);

    auto shaderResourceTable = Retina::CShaderResourceTable::Make(*device);

    auto mainPipeline = Retina::CRayTracingPipeline::Make(*device, {
        .Name = "MainPipeline",
        .RayGenShader = RETINA_MAIN_SHADER_PATH "/RayTracing/Main.rgen.glsl",
        .HitGroupShaders = {
            {
                .ClosestHit = RETINA_MAIN_SHADER_PATH "/RayTracing/Main.rchit.glsl",
                .AnyHit = RETINA_MAIN_SHADER_PATH "/RayTracing/Main.rahit.glsl",
            },
        },
        .MissShaders = {
            RETINA_MAIN_SHADER_PATH "/RayTracing/Main.rmiss.glsl",
        },
        .DescriptorLayouts = { {
            shaderResourceTable->GetDescriptorLayout()
        } },
    });
    auto tonemapImage = Retina::CImage::Make(*device, {
        .Name = "TonemapImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                 Retina::EImageUsage::E_TRANSFER_SRC,
        .Format = Retina::EResourceFormat::E_R8G8B8A8_UNORM,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto tonemapPipeline = Retina::CGraphicsPipeline::Make(*device, {
        .Name = "TonemapPipeline",
        .VertexShader = RETINA_MAIN_SHADER_PATH "/FullscreenTriangle.vert.glsl",
        .FragmentShader = RETINA_MAIN_SHADER_PATH "/Tonemap.frag.glsl",
        .DescriptorLayouts = { {
            shaderResourceTable->GetDescriptorLayout()
        } },
        .DynamicState = { {
            Retina::EDynamicState::E_VIEWPORT,
            Retina::EDynamicState::E_SCISSOR,
        } },
        .RenderingInfo = { {
            .ColorAttachmentFormats = {
                tonemapImage->GetFormat()
            }
        } }
    });
    auto mainImage = shaderResourceTable->MakeStorageImage({
        .Name = "MainImage",
        .Width = swapchain->GetWidth(),
        .Height = swapchain->GetHeight(),
        .Usage = Retina::EImageUsage::E_STORAGE,
        .Format = Retina::EResourceFormat::E_R32G32B32A32_SFLOAT,
        .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
    });
    auto cameraBuffers = shaderResourceTable->MakeStorageBuffer<SViewInfo>(2, {
        .Name = "ViewInfoBuffer",
        .Heap = Retina::EMemoryProperty::E_DEVICE_MAPPABLE,
        .Capacity = 1,
    });

    const auto model = CModel::Make("../test/bistro.glb");
    const auto textures = MakeTexturesFromModel(*device, model, *shaderResourceTable);
    const auto blases = MakeBlasesFromModel(*device, model);
    const auto tlas = MakeTlasFromModel(*device, model, *shaderResourceTable, blases);
    const auto vertexBuffer = MakeVertexBufferFromModel(model, *shaderResourceTable);
    const auto indexBuffer = MakeIndexBufferFromModel(model, *shaderResourceTable);
    const auto objectInfoBuffer = MakeObjectInfoBufferFromModel(model, *shaderResourceTable);
    const auto materialBuffer = MakeMaterialBufferFromModel(model, *shaderResourceTable);
    const auto mainSampler = shaderResourceTable->MakeSampler({
        .Name = "MainSampler",
        .Filter = { Retina::ESamplerFilter::E_LINEAR },
        .Address = { Retina::ESamplerAddressMode::E_REPEAT },
        .MipmapMode = Retina::ESamplerMipmapMode::E_LINEAR,
        .BorderColor = Retina::ESamplerBorderColor::E_FLOAT_TRANSPARENT_BLACK,
        .AnisotropyEnable = true,
        .Anisotropy = 16.0f,
    });
    auto camera = CCamera::Make(window->GetInput());
    auto frameCounter = 0_u32;

    const auto resize = [&] {
        device->WaitIdle();
        window->UpdateViewportExtent();
        while (window->GetWidth() == 0 || window->GetHeight() == 0) {
            Retina::Platform::WaitEvents();
            window->UpdateViewportExtent();
        }

        swapchain = Retina::CSwapchain::Recreate(std::move(swapchain));
        mainImage.Destroy();
        mainImage = shaderResourceTable->MakeStorageImage({
            .Name = "MainImage",
            .Width = swapchain->GetWidth(),
            .Height = swapchain->GetHeight(),
            .Usage = Retina::EImageUsage::E_STORAGE,
            .Format = Retina::EResourceFormat::E_R32G32B32A32_SFLOAT,
            .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
        });
        tonemapImage = Retina::CImage::Make(*device, {
            .Name = "TonemapImage",
            .Width = swapchain->GetWidth(),
            .Height = swapchain->GetHeight(),
            .Usage = Retina::EImageUsage::E_COLOR_ATTACHMENT |
                     Retina::EImageUsage::E_TRANSFER_SRC,
            .Format = Retina::EResourceFormat::E_R8G8B8A8_UNORM,
            .ViewInfo = Retina::Constant::DEFAULT_IMAGE_VIEW_INFO,
        });
        frameCounter = 0;
    };

    auto lastTime = 0.0f;
    auto previousViewInfo = SViewInfo();
    while (window->IsOpen()) {
        const auto currentFrameIndex = frameTimeline->WaitForNextTimelineValue();
        shaderResourceTable->Update();
        if (!swapchain->AcquireNextImage(*imageAvailableSemaphores[currentFrameIndex])) {
            resize();
            continue;
        }
        const auto& swapchainImage = swapchain->GetCurrentImage();
        const auto currentTime = Retina::Platform::GetTimeSinceInit();
        const auto deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        window->GetInput().Update();
        camera.Update(deltaTime);

        auto& currentCameraBuffer = cameraBuffers[currentFrameIndex];

        {
            const auto aspectRatio =
                static_cast<float32>(swapchain->GetWidth()) /
                static_cast<float32>(swapchain->GetHeight());
            auto projection = glm::perspective(glm::radians(60.0f), aspectRatio, 0.1f, 10000.0f);
            projection[1][1] *= -1;
            const auto view = camera.GetViewMatrix();
            auto viewInfo = SViewInfo();
            viewInfo.InvProjection = glm::inverse(projection);
            viewInfo.InvView = glm::inverse(view);
            viewInfo.InvProjView = glm::inverse(projection * view);
            viewInfo.Position = glm::vec4(camera.GetPosition(), 1.0f);
            currentCameraBuffer->Write(viewInfo);
            if (std::memcmp(&viewInfo, &previousViewInfo, sizeof(SViewInfo)) != 0) {
                frameCounter = 0;
            }
            previousViewInfo = viewInfo;
        }

        auto& currentCommandBuffer = *commandBuffers[currentFrameIndex];
        currentCommandBuffer.GetCommandPool().Reset();
        currentCommandBuffer.Begin();
        if (frameCounter == 0) {
            currentCommandBuffer.ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_NONE,
                .DestStage = Retina::EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            });
        } else {
            currentCommandBuffer.ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_FRAGMENT_SHADER,
                .DestStage = Retina::EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_READ |
                              Retina::EResourceAccess::E_SHADER_STORAGE_WRITE,
                .OldLayout = Retina::EImageLayout::E_GENERAL,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            });
        }
        currentCommandBuffer
            .BindPipeline(*mainPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                frameCounter,
                mainImage.GetHandle(),
                currentCameraBuffer.GetHandle(),
                vertexBuffer.GetHandle(),
                indexBuffer.GetHandle(),
                objectInfoBuffer.GetHandle(),
                materialBuffer.GetHandle(),
                mainSampler.GetHandle(),
                tlas.GetHandle()
            )
            .TraceRays(swapchain->GetWidth(), swapchain->GetHeight())
            .ImageMemoryBarrier({
                .Image = *mainImage,
                .SourceStage = Retina::EPipelineStage::E_RAY_TRACING_SHADER_KHR,
                .DestStage = Retina::EPipelineStage::E_FRAGMENT_SHADER,
                .SourceAccess = Retina::EResourceAccess::E_SHADER_STORAGE_WRITE,
                .DestAccess = Retina::EResourceAccess::E_SHADER_STORAGE_READ,
                .OldLayout = Retina::EImageLayout::E_GENERAL,
                .NewLayout = Retina::EImageLayout::E_GENERAL,
            })
            .ImageMemoryBarrier({
                .Image = *tonemapImage,
                .SourceStage = Retina::EPipelineStage::E_NONE,
                .DestStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
            })
            .BeginRendering({
                .Name = "TonemapPass",
                .ColorAttachments = {
                    {
                        .Image = *tonemapImage,
                        .LoadOperator = Retina::EAttachmentLoadOperator::E_DONT_CARE,
                        .StoreOperator = Retina::EAttachmentStoreOperator::E_STORE,
                    }
                },
            })
            .SetViewport()
            .SetScissor()
            .BindPipeline(*tonemapPipeline)
            .BindDescriptorSet(shaderResourceTable->GetDescriptorSet())
            .PushConstants(
                mainImage.GetHandle()
            )
            .Draw(3, 1, 0, 0)
            .EndRendering()
            .ImageMemoryBarrier({
                .Image = *tonemapImage,
                .SourceStage = Retina::EPipelineStage::E_COLOR_ATTACHMENT_OUTPUT,
                .DestStage = Retina::EPipelineStage::E_BLIT,
                .SourceAccess = Retina::EResourceAccess::E_COLOR_ATTACHMENT_WRITE,
                .DestAccess = Retina::EResourceAccess::E_TRANSFER_READ,
                .OldLayout = Retina::EImageLayout::E_COLOR_ATTACHMENT_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_TRANSFER_SRC_OPTIMAL,
            })
            .BeginNamedRegion("SwapchainCopy")
            .ImageMemoryBarrier({
                .Image = swapchainImage,
                .SourceStage = Retina::EPipelineStage::E_TOP_OF_PIPE,
                .DestStage = Retina::EPipelineStage::E_TRANSFER,
                .SourceAccess = Retina::EResourceAccess::E_NONE,
                .DestAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .OldLayout = Retina::EImageLayout::E_UNDEFINED,
                .NewLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
            })
            .BlitImage(*tonemapImage, swapchainImage, {})
            .ImageMemoryBarrier({
                .Image = swapchainImage,
                .SourceStage = Retina::EPipelineStage::E_TRANSFER,
                .DestStage = Retina::EPipelineStage::E_BOTTOM_OF_PIPE,
                .SourceAccess = Retina::EResourceAccess::E_TRANSFER_WRITE,
                .DestAccess = Retina::EResourceAccess::E_NONE,
                .OldLayout = Retina::EImageLayout::E_TRANSFER_DST_OPTIMAL,
                .NewLayout = Retina::EImageLayout::E_PRESENT_SRC_KHR,
            })
            .EndNamedRegion()
            .End();

        device->GetGraphicsQueue().Submit({
            .CommandBuffers = { currentCommandBuffer },
            .WaitSemaphores = {
                { *imageAvailableSemaphores[currentFrameIndex], Retina::EPipelineStage::E_TRANSFER },
            },
            .SignalSemaphores = {
                { *presentReadySemaphores[currentFrameIndex], Retina::EPipelineStage::E_TRANSFER },
            },
            .Timeline = *frameTimeline,
        });

        {
            const auto success = device->GetGraphicsQueue().Present({
                .Swapchain = *swapchain,
                .WaitSemaphores = {
                    *presentReadySemaphores[currentFrameIndex],
                },
            });
            if (!success) {
                resize();
                continue;
            }
        }
        frameCounter++;
        window->GetInput().Update();
        Retina::Platform::PollEvents();
        RETINA_MARK_FRAME();
    }
    device->GetGraphicsQueue().WaitIdle();
    return 0;
}
