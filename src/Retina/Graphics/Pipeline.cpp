#include <Retina/Graphics/DescriptorLayout.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Pipeline.hpp>

#include <volk.h>

#include <shaderc/shaderc.hpp>

#include <spirv_glsl.hpp>

#include <mio/mmap.hpp>

#include <utility>
#include <vector>
#include <ranges>
#include <span>

namespace Retina::Graphics {
  namespace Details {
    class CShaderIncludeResult : public shaderc_include_result {
    public:
      using ShadercIncludeResult = shaderc_include_result;

      CShaderIncludeResult(std::filesystem::path path) noexcept
        : ShadercIncludeResult(),
          _path(std::move(path)),
          _filename(_path.filename().generic_string()),
          _file(_path.generic_string())
      {
        RETINA_PROFILE_SCOPED();
        ShadercIncludeResult::content = _file.data();
        ShadercIncludeResult::content_length = _file.size();
        ShadercIncludeResult::source_name = _filename.c_str();
        ShadercIncludeResult::source_name_length = _filename.size();
      }

      ~CShaderIncludeResult() noexcept = default;

    private:
      std::filesystem::path _path;
      std::string _filename;
      mio::mmap_source _file;
    };

    class CShaderIncludeResolver : public shaderc::CompileOptions::IncluderInterface {
    public:
      using ShadercIncluderInterface = shaderc::CompileOptions::IncluderInterface;

      CShaderIncludeResolver(std::span<const std::filesystem::path> includeDirectories) noexcept
        : ShadercIncluderInterface(),
          _includeDirectories(includeDirectories.begin(), includeDirectories.end())
      {
        RETINA_PROFILE_SCOPED();
      }

      ~CShaderIncludeResolver() override = default;

      auto GetInclude(
        const char* requestedSource,
        shaderc_include_type,
        const char*,
        size_t
      ) noexcept -> shaderc_include_result* override {
        RETINA_PROFILE_SCOPED();
        auto includePath = std::filesystem::path(requestedSource);
        for (const auto& includeDirectory : _includeDirectories) {
          auto path = (includeDirectory / includePath).make_preferred();
          if (std::filesystem::exists(path)) {
            return new CShaderIncludeResult(path);
          }
        }
        if (includePath.is_absolute()) {
          if (std::filesystem::exists(includePath)) {
            return new CShaderIncludeResult(includePath);
          }
        }
        RETINA_GRAPHICS_PANIC_WITH("Failed to find include '{}'", requestedSource);
        return nullptr;
      }

      void ReleaseInclude(shaderc_include_result* data) override {
        RETINA_PROFILE_SCOPED();
        delete static_cast<CShaderIncludeResult*>(data);
      }

    private:
      std::vector<std::filesystem::path> _includeDirectories;
    };

    RETINA_NODISCARD RETINA_INLINE auto GetShaderKindFrom(EShaderStageFlag stage) noexcept -> shaderc_shader_kind {
      RETINA_PROFILE_SCOPED();
      switch (stage) {
        case EShaderStageFlag::E_VERTEX: return shaderc_glsl_vertex_shader;
        case EShaderStageFlag::E_TESSELLATION_CONTROL: return shaderc_glsl_tess_control_shader;
        case EShaderStageFlag::E_TESSELLATION_EVALUATION: return shaderc_glsl_tess_evaluation_shader;
        case EShaderStageFlag::E_GEOMETRY: return shaderc_glsl_geometry_shader;
        case EShaderStageFlag::E_FRAGMENT: return shaderc_glsl_fragment_shader;
        case EShaderStageFlag::E_COMPUTE: return shaderc_glsl_compute_shader;
        case EShaderStageFlag::E_RAYGEN_KHR: return shaderc_glsl_raygen_shader;
        case EShaderStageFlag::E_ANY_HIT_KHR: return shaderc_glsl_anyhit_shader;
        case EShaderStageFlag::E_CLOSEST_HIT_KHR: return shaderc_glsl_closesthit_shader;
        case EShaderStageFlag::E_MISS_KHR: return shaderc_glsl_miss_shader;
        case EShaderStageFlag::E_INTERSECTION_KHR: return shaderc_glsl_intersection_shader;
        case EShaderStageFlag::E_CALLABLE_KHR: return shaderc_glsl_callable_shader;
        case EShaderStageFlag::E_TASK_EXT: return shaderc_glsl_task_shader;
        case EShaderStageFlag::E_MESH_EXT: return shaderc_glsl_mesh_shader;
        default: return shaderc_glsl_infer_from_source;
      }
    }

    auto CompileShaderFromSource(
      const std::filesystem::path& path,
      std::span<const std::filesystem::path> includeDirectories,
      EShaderStageFlag stage
    ) noexcept -> std::vector<uint32> {
      RETINA_PROFILE_SCOPED();
      const auto root = std::filesystem::path(RETINA_MAIN_SHADER_DIRECTORY);
      auto includeDirectoriesWithRoot = std::vector(
        includeDirectories.begin(),
        includeDirectories.end()
      );
      includeDirectoriesWithRoot.emplace_back(root);
      if (!std::filesystem::exists(path)) {
        RETINA_GRAPHICS_PANIC_WITH("Shader '{}' does not exist", path.generic_string());
      }
      const auto shaderSource = mio::mmap_source(path.generic_string());
      auto compiler = shaderc::Compiler();
      auto compilerOptions = shaderc::CompileOptions();
      compilerOptions.SetGenerateDebugInfo();
      compilerOptions.SetOptimizationLevel(shaderc_optimization_level_zero);
      compilerOptions.SetIncluder(std::make_unique<Details::CShaderIncludeResolver>(includeDirectoriesWithRoot));
      compilerOptions.SetSourceLanguage(shaderc_source_language_glsl);
      compilerOptions.SetTargetEnvironment(shaderc_target_env_vulkan, shaderc_env_version_vulkan_1_3);
      compilerOptions.SetTargetSpirv(shaderc_spirv_version_1_6);
      compilerOptions.SetForcedVersionProfile(460, shaderc_profile_core);
      compilerOptions.SetPreserveBindings(true);

      auto spirv = compiler.CompileGlslToSpv(
        shaderSource.data(),
        shaderSource.size(),
        GetShaderKindFrom(stage),
        path.generic_string().c_str(),
        compilerOptions
      );
      if (spirv.GetCompilationStatus() != shaderc_compilation_status_success) {
        RETINA_GRAPHICS_PANIC_WITH(
          "Failed to compile shader '{}': {}",
          path.generic_string(),
          spirv.GetErrorMessage()
        );
      }
      return { spirv.cbegin(), spirv.cend() };
    }

    auto MakeShaderModule(const CDevice& device, std::span<const uint32> spirv) noexcept -> VkShaderModule {
      RETINA_PROFILE_SCOPED();
      auto shaderModuleCreateInfo = VkShaderModuleCreateInfo(VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
      shaderModuleCreateInfo.codeSize = spirv.size_bytes();
      shaderModuleCreateInfo.pCode = spirv.data();
      auto shaderModule = VkShaderModule();
      RETINA_GRAPHICS_VULKAN_CHECK(
        vkCreateShaderModule(
          device.GetHandle(),
          &shaderModuleCreateInfo,
          nullptr,
          &shaderModule
        )
      );
      return shaderModule;
    }

    auto ExecutionModelToShaderStage(const spirv_cross::CompilerGLSL& compiler) noexcept -> EShaderStageFlag {
      RETINA_PROFILE_SCOPED();
      switch (compiler.get_execution_model()) {
        case spv::ExecutionModelVertex: return EShaderStageFlag::E_VERTEX;
        case spv::ExecutionModelTessellationControl: return EShaderStageFlag::E_TESSELLATION_CONTROL;
        case spv::ExecutionModelTessellationEvaluation: return EShaderStageFlag::E_TESSELLATION_EVALUATION;
        case spv::ExecutionModelGeometry: return EShaderStageFlag::E_GEOMETRY;
        case spv::ExecutionModelFragment: return EShaderStageFlag::E_FRAGMENT;
        case spv::ExecutionModelGLCompute: RETINA_FALLTHROUGH;
        case spv::ExecutionModelKernel: return EShaderStageFlag::E_COMPUTE;
        case spv::ExecutionModelTaskNV: return EShaderStageFlag::E_TASK_NV;
        case spv::ExecutionModelMeshNV: return EShaderStageFlag::E_MESH_NV;
        case spv::ExecutionModelRayGenerationKHR: return EShaderStageFlag::E_RAYGEN_KHR;
        case spv::ExecutionModelIntersectionKHR: return EShaderStageFlag::E_INTERSECTION_KHR;
        case spv::ExecutionModelAnyHitKHR: return EShaderStageFlag::E_ANY_HIT_KHR;
        case spv::ExecutionModelClosestHitKHR: return EShaderStageFlag::E_CLOSEST_HIT_KHR;
        case spv::ExecutionModelMissKHR: return EShaderStageFlag::E_MISS_KHR;
        case spv::ExecutionModelCallableKHR: return EShaderStageFlag::E_CALLABLE_KHR;
        case spv::ExecutionModelTaskEXT: return EShaderStageFlag::E_TASK_EXT;
        case spv::ExecutionModelMeshEXT: return EShaderStageFlag::E_MESH_EXT;
        default: return {};
      }
    }

    auto ReflectPushConstantRange(
      std::span<const spirv_cross::CompilerGLSL* const> compilers
    ) noexcept -> SPipelinePushConstantInfo {
      RETINA_PROFILE_SCOPED();
      auto pushConstantInfo = SPipelinePushConstantInfo();
      for (const auto* compiler : compilers) {
        if (!compiler) {
          continue;
        }
        const auto resources = compiler->get_shader_resources();
        if (resources.push_constant_buffers.empty()) {
          continue;
        }
        const auto& pushConstantBuffer = resources.push_constant_buffers.back();
        const auto& pushConstantType = compiler->get_type(pushConstantBuffer.type_id);
        const auto pushConstantSize = compiler->get_declared_struct_size(pushConstantType);
        const auto stage = ExecutionModelToShaderStage(*compiler);
        if (pushConstantInfo.Size == 0) {
          pushConstantInfo.Stages = stage;
          pushConstantInfo.Size = pushConstantSize;
        } else {
          RETINA_ASSERT_WITH(pushConstantInfo.Size == pushConstantSize, "Push constant size mismatch");
          pushConstantInfo.Stages |= stage;
        }
      }
      return pushConstantInfo;
    }

    auto MakeDescriptorLayoutHandles(
      std::span<const Core::CReferenceWrapper<const CDescriptorLayout>> layouts
    ) noexcept -> std::vector<VkDescriptorSetLayout> {
      RETINA_PROFILE_SCOPED();
      auto descriptorLayoutHandles = std::vector<VkDescriptorSetLayout>();
      descriptorLayoutHandles.reserve(layouts.size());
      for (const auto& layout : layouts) {
        descriptorLayoutHandles.push_back(layout->GetHandle());
      }
      return descriptorLayoutHandles;
    }
  }

  IPipeline::~IPipeline() noexcept {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      vkDestroyPipeline(GetDevice().GetHandle(), _handle, nullptr);
      vkDestroyPipelineLayout(GetDevice().GetHandle(), _layout.Handle, nullptr);
    }
  }

  auto IPipeline::GetHandle() const noexcept -> VkPipeline {
    RETINA_PROFILE_SCOPED();
    return _handle;
  }

  auto IPipeline::GetLayout() const noexcept -> const SPipelineLayout& {
    RETINA_PROFILE_SCOPED();
    return _layout;
  }

  auto IPipeline::GetType() const noexcept -> EPipelineType {
    RETINA_PROFILE_SCOPED();
    return _type;
  }

  auto IPipeline::GetDevice() const noexcept -> const CDevice& {
    RETINA_PROFILE_SCOPED();
    return *_device;
  }

  auto IPipeline::GetLayoutHandle() const noexcept -> VkPipelineLayout {
    RETINA_PROFILE_SCOPED();
    return _layout.Handle;
  }

  auto IPipeline::GetLayoutPushConstantInfo() const noexcept -> const SPipelinePushConstantInfo& {
    RETINA_PROFILE_SCOPED();
    return _layout.PushConstant;
  }

  auto IPipeline::GetBindPoint() const noexcept -> EPipelineBindPoint {
    RETINA_PROFILE_SCOPED();
    return [this] {
      switch (_type) {
        case EPipelineType::E_GRAPHICS: RETINA_FALLTHROUGH;
        case EPipelineType::E_MESH_SHADING: return EPipelineBindPoint::E_GRAPHICS;
        case EPipelineType::E_COMPUTE: return EPipelineBindPoint::E_COMPUTE;
        case EPipelineType::E_RAY_TRACING: return EPipelineBindPoint::E_RAY_TRACING_KHR;
        default: return EPipelineBindPoint::E_GRAPHICS;
      }
    }();
  }

  IPipeline::IPipeline(EPipelineType type) noexcept
    : _type(type)
  {
    RETINA_PROFILE_SCOPED();
  }
}
