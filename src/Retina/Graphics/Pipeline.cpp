#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Pipeline.hpp>

#include <volk.h>

#include <shaderc/shaderc.hpp>
#include <mio/mmap.hpp>

#include <utility>
#include <vector>
#include <ranges>
#include <format>
#include <span>

namespace Retina {
    namespace Shc = shaderc;

    namespace Private {
        class CShaderIncludeResult : public shaderc_include_result {
        public:
            using Self = CShaderIncludeResult;
            using IncludeResult = shaderc_include_result;

            CShaderIncludeResult(fs::path path) noexcept
                : IncludeResult(),
                  _path(std::move(path)),
                  _filename(_path.filename().generic_string()),
                  _file(_path.generic_string()) {
                RETINA_PROFILE_SCOPED();

                IncludeResult::content = _file.data();
                IncludeResult::content_length = _file.size();
                IncludeResult::source_name = _filename.c_str();
                IncludeResult::source_name_length = _filename.size();
                IncludeResult::user_data = nullptr;
            }

            ~CShaderIncludeResult() noexcept = default;

        private:
            fs::path _path;
            std::string _filename;
            mio::mmap_source _file;
        };

        class CShaderIncluder : public Shc::CompileOptions::IncluderInterface {
        public:
            using Self = CShaderIncluder;
            using IncluderInterface = Shc::CompileOptions::IncluderInterface;

            CShaderIncluder(std::span<const fs::path> includeDirectories) noexcept
                : _includeDirectories(includeDirectories.begin(), includeDirectories.end()) {
                RETINA_PROFILE_SCOPED();
            }

            ~CShaderIncluder() noexcept override = default;

            auto GetInclude(const char* requested, shaderc_include_type, const char*, size_t) noexcept -> shaderc_include_result* override {
                RETINA_PROFILE_SCOPED();
                auto includePath = fs::path(requested);
                for (const auto& includeDirectory : _includeDirectories) {
                    if (auto result = GetIncludeResult((includeDirectory / includePath).make_preferred())) {
                        return result;
                    }
                }
                if (includePath.is_absolute()) {
                    return GetIncludeResult(includePath.make_preferred());
                }
                return nullptr;
            }

            auto ReleaseInclude(shaderc_include_result* data) noexcept -> void override {
                RETINA_PROFILE_SCOPED();
                delete static_cast<CShaderIncludeResult*>(data);
            }

        private:
            static auto GetIncludeResult(const fs::path& path) noexcept -> CShaderIncludeResult* {
                RETINA_PROFILE_SCOPED();
                if (!fs::exists(path)) {
                    return nullptr;
                }
                return new CShaderIncludeResult(path);
            }

            std::vector<fs::path> _includeDirectories = {};
        };
    }

    RETINA_NODISCARD static auto ToCompilerShaderKind(EShaderStage shaderStage) noexcept -> shaderc_shader_kind {
        RETINA_PROFILE_SCOPED();
        switch (shaderStage) {
            case EShaderStage::E_VERTEX: return shaderc_glsl_vertex_shader;
            case EShaderStage::E_TESSELLATION_CONTROL: return shaderc_glsl_tess_control_shader;
            case EShaderStage::E_TESSELLATION_EVALUATION: return shaderc_glsl_tess_evaluation_shader;
            case EShaderStage::E_GEOMETRY: return shaderc_glsl_geometry_shader;
            case EShaderStage::E_FRAGMENT: return shaderc_glsl_fragment_shader;
            case EShaderStage::E_COMPUTE: return shaderc_glsl_compute_shader;
            case EShaderStage::E_RAYGEN_KHR: return shaderc_glsl_raygen_shader;
            case EShaderStage::E_ANY_HIT_KHR: return shaderc_glsl_anyhit_shader;
            case EShaderStage::E_CLOSEST_HIT_KHR: return shaderc_glsl_closesthit_shader;
            case EShaderStage::E_MISS_KHR: return shaderc_glsl_miss_shader;
            case EShaderStage::E_INTERSECTION_KHR: return shaderc_glsl_intersection_shader;
            case EShaderStage::E_CALLABLE_KHR: return shaderc_glsl_callable_shader;
            case EShaderStage::E_TASK_EXT: return shaderc_glsl_task_shader;
            case EShaderStage::E_MESH_EXT: return shaderc_glsl_mesh_shader;
            default: return shaderc_glsl_infer_from_source;
        }
    }

    auto CompileShaderFromSource(
        const CDevice& device,
        const fs::path& path,
        std::span<const fs::path> includeDirectories,
        EShaderStage shaderStage
    ) noexcept -> std::vector<uint32> {
        RETINA_PROFILE_SCOPED();
        const auto rootShaderPath = fs::path(RETINA_MAIN_SHADER_PATH);
        const auto shaderPath = (rootShaderPath / path).make_preferred();
        auto includeDirectoriesWithRoot = std::vector<fs::path>(includeDirectories.begin(), includeDirectories.end());
        includeDirectoriesWithRoot.emplace_back(rootShaderPath);
        if (!fs::exists(shaderPath)) {
            RETINA_PANIC_WITH(device.GetLogger(), "Shader \"{}\" does not exist", shaderPath.generic_string());
        }
        const auto shaderSource = mio::mmap_source(shaderPath.string());
        auto compiler = Shc::Compiler();
        auto compilerOptions = Shc::CompileOptions();
        compilerOptions.SetGenerateDebugInfo();
        compilerOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
        compilerOptions.SetIncluder(std::make_unique<Private::CShaderIncluder>(includeDirectoriesWithRoot));
        compilerOptions.SetSourceLanguage(shaderc_source_language_glsl);
        compilerOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
        compilerOptions.SetTargetSpirv(shaderc_spirv_version_1_6);
        auto spirv = compiler.CompileGlslToSpv(
            shaderSource.data(),
            shaderSource.size(),
            ToCompilerShaderKind(shaderStage),
            shaderPath.generic_string().c_str(),
            "main",
            compilerOptions
        );
        if (spirv.GetCompilationStatus() != shaderc_compilation_status_success) {
            RETINA_LOG_ERROR(
                device.GetLogger(),
                "Failed to compile shader \"{}\":",
                shaderPath.generic_string()
            );
            RETINA_PANIC_WITH(
                device.GetLogger(),
                "{}",
                spirv.GetErrorMessage()
            );
        }
        return { spirv.cbegin(), spirv.cend() };
    }

    IPipeline::IPipeline(EPipelineType type) noexcept : _type(type) {
        RETINA_PROFILE_SCOPED();
    }

    IPipeline::~IPipeline() noexcept {
        RETINA_PROFILE_SCOPED();
        RETINA_LOG_INFO(_device->GetLogger(), "Destroying Pipeline: \"{}\"", GetDebugName());
        vkDestroyPipelineLayout(_device->GetHandle(), _layout, nullptr);
        vkDestroyPipeline(_device->GetHandle(), _handle, nullptr);
    }

    auto IPipeline::GetHandle() const noexcept -> VkPipeline {
        RETINA_PROFILE_SCOPED();
        return _handle;
    }

    auto IPipeline::GetLayout() const noexcept -> VkPipelineLayout {
        RETINA_PROFILE_SCOPED();
        return _layout;
    }

    auto IPipeline::GetType() const noexcept -> EPipelineType {
        RETINA_PROFILE_SCOPED();
        return _type;
    }

    auto IPipeline::SetDebugName(std::string_view name) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        if (name.empty()) {
            return;
        }
        INativeDebugName::SetDebugName(name);
        auto info = VkDebugUtilsObjectNameInfoEXT(VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT);
        info.objectType = VK_OBJECT_TYPE_PIPELINE;
        info.objectHandle = reinterpret_cast<uint64>(_handle);
        info.pObjectName = name.data();

        RETINA_VULKAN_CHECK(
            _device->GetLogger(),
            vkSetDebugUtilsObjectNameEXT(
                _device->GetHandle(),
                &info
            )
        );
    }
}
