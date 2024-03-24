#include <Retina/Graphics/NVIDIA/NvidiaDlssFeature.hpp>
#include <Retina/Graphics/CommandBuffer.hpp>
#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Instance.hpp>
#include <Retina/Graphics/Image.hpp>
#include <Retina/Graphics/ImageView.hpp>
#include <Retina/Graphics/Logger.hpp>
#include <Retina/Graphics/Macros.hpp>
#include <Retina/Graphics/Queue.hpp>

#include <volk.h>
#include <vulkan/vulkan.h>

#include <nvsdk_ngx_vk.h>
#include <nvsdk_ngx_helpers_vk.h>

#include <filesystem>

#define RETINA_NVIDIA_PROJECT_ID "5F9FC80E-4C3D-4327-B4ED-86D84BC46457"
#define RETINA_NVIDIA_PROJECT_VERSION "0.1.0"
#define RETINA_NVIDIA_PROJECT_LOGS_DIRECTORY L"./Logs"

#define RETINA_NVIDIA_CHECK(x)                                                        \
  do {                                                                                \
    const auto _e = (x);                                                              \
    if (NVSDK_NGX_FAILED(_e)) {                                                       \
      RETINA_GRAPHICS_PANIC_WITH("NVIDIA DLSS Failure: {}", std::to_underlying(_e));  \
    }                                                                                 \
  } while (false)

namespace Retina::Graphics {
  namespace Details {
    RETINA_NODISCARD RETINA_INLINE constexpr auto MakeNvidiaDlssFeatureCommonInfo() noexcept -> NVSDK_NGX_FeatureCommonInfo {
      auto info = NVSDK_NGX_FeatureCommonInfo();
      info.LoggingInfo.LoggingCallback = [](const char* message, NVSDK_NGX_Logging_Level, NVSDK_NGX_Feature feature) noexcept -> void {
        RETINA_PROFILE_SCOPED();
        RETINA_CORE_INFO("NVIDIA [{}]: {}", std::to_underlying(feature), message);
      };
      info.LoggingInfo.MinimumLoggingLevel = NVSDK_NGX_LOGGING_LEVEL_ON;
      return info;
    }

    RETINA_NODISCARD RETINA_INLINE constexpr auto MakeNvidiaDlssFeatureDiscoveryInfo(
      const NVSDK_NGX_FeatureCommonInfo& commonInfo
    ) noexcept -> NVSDK_NGX_FeatureDiscoveryInfo {
      auto info = NVSDK_NGX_FeatureDiscoveryInfo();
      info.SDKVersion = NVSDK_NGX_Version_API;
      info.FeatureID = NVSDK_NGX_Feature_SuperSampling;
      info.Identifier.IdentifierType = NVSDK_NGX_Application_Identifier_Type_Project_Id;
      info.Identifier.v.ProjectDesc.ProjectId = RETINA_NVIDIA_PROJECT_ID;
      info.Identifier.v.ProjectDesc.EngineType = NVSDK_NGX_ENGINE_TYPE_CUSTOM;
      info.Identifier.v.ProjectDesc.EngineVersion = RETINA_NVIDIA_PROJECT_VERSION;
      info.ApplicationDataPath = RETINA_NVIDIA_PROJECT_LOGS_DIRECTORY;
      info.FeatureInfo = &commonInfo;
      return info;
    }

    RETINA_NODISCARD RETINA_INLINE auto ToNvidiaResource(const CImage& image) noexcept -> NVSDK_NGX_Resource_VK {
      RETINA_PROFILE_SCOPED();
      const auto handle = image.GetHandle();
      const auto viewHandle = image.GetView().GetHandle();
      const auto format = AsEnumCounterpart(image.GetFormat());
      const auto subresourceRange = MakeNativeImageSubresourceRangeFrom(image);

      return {
        .Resource = {
          .ImageViewInfo = {
            .ImageView = viewHandle,
            .Image = handle,
            .SubresourceRange = subresourceRange,
            .Format = format,
            .Width = image.GetWidth(),
            .Height = image.GetHeight(),
          }
        },
        .Type = NVSDK_NGX_RESOURCE_VK_TYPE_VK_IMAGEVIEW,
        .ReadWrite = Core::IsFlagEnabled(image.GetUsage(), EImageUsageFlag::E_STORAGE),
      };
    }

    constexpr static auto NVIDIA_DLSS_FEATURE_COMMON_INFO = MakeNvidiaDlssFeatureCommonInfo();
    constexpr static auto NVIDIA_DLSS_FEATURE_DISCOVERY_INFO = MakeNvidiaDlssFeatureDiscoveryInfo(NVIDIA_DLSS_FEATURE_COMMON_INFO);
  }

  CNvidiaDlssFeature::~CNvidiaDlssFeature() noexcept {
    RETINA_PROFILE_SCOPED();
    Shutdown();
    NVSDK_NGX_VULKAN_DestroyParameters(_parameters);
    NVSDK_NGX_VULKAN_Shutdown1(nullptr);
  }

  auto CNvidiaDlssFeature::Make(const CDevice& device) noexcept -> Core::CUniquePtr<CNvidiaDlssFeature> {
    auto self = Core::CUniquePtr(new CNvidiaDlssFeature());

    if (!std::filesystem::exists(RETINA_NVIDIA_PROJECT_LOGS_DIRECTORY)) {
      std::filesystem::create_directories(RETINA_NVIDIA_PROJECT_LOGS_DIRECTORY);
    }

    RETINA_NVIDIA_CHECK(
      NVSDK_NGX_VULKAN_Init_with_ProjectID(
        RETINA_NVIDIA_PROJECT_ID,
        NVSDK_NGX_ENGINE_TYPE_CUSTOM,
        RETINA_NVIDIA_PROJECT_VERSION,
        RETINA_NVIDIA_PROJECT_LOGS_DIRECTORY,
        device.GetInstance().GetHandle(),
        device.GetPhysicalDevice(),
        device.GetHandle()
      )
    );

    RETINA_NVIDIA_CHECK(NVSDK_NGX_VULKAN_GetCapabilityParameters(&self->_parameters));

    auto isDriverUpdateRequired = 0_i32;
    auto minDriverVersionMajor = 0_i32;
    auto minDriverVersionMinor = 0_i32;

    RETINA_NVIDIA_CHECK(
      self->_parameters->Get(
        NVSDK_NGX_Parameter_SuperSampling_NeedsUpdatedDriver,
        &isDriverUpdateRequired
      )
    );

    if (isDriverUpdateRequired) {
      RETINA_NVIDIA_CHECK(
        self->_parameters->Get(
          NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMajor,
          &minDriverVersionMajor
        )
      );
      RETINA_NVIDIA_CHECK(
        self->_parameters->Get(
          NVSDK_NGX_Parameter_SuperSampling_MinDriverVersionMinor,
          &minDriverVersionMinor
        )
      );
      RETINA_GRAPHICS_PANIC_WITH(
        "NVIDIA DLSS requires a driver update to version {}.{}",
        minDriverVersionMajor,
        minDriverVersionMinor
      );
    }

    auto isDlssAvailable = 0_i32;
    RETINA_NVIDIA_CHECK(self->_parameters->Get(NVSDK_NGX_Parameter_SuperSampling_Available, &isDlssAvailable));
    if (!isDlssAvailable) {
      RETINA_GRAPHICS_PANIC_WITH("NVIDIA DLSS is not available on this system");
    }

    self->_device = device.ToArcPtr();
    return self;
  }

  auto CNvidiaDlssFeature::Initialize(const SNvidiaDlssInitializeInfo& initializeInfo) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto dlssFlags =
      NVSDK_NGX_DLSS_Feature_Flags_MVLowRes |
      (initializeInfo.IsHDR ? NVSDK_NGX_DLSS_Feature_Flags_IsHDR : 0) |
      (initializeInfo.IsReverseDepth ? NVSDK_NGX_DLSS_Feature_Flags_DepthInverted : 0) |
      (initializeInfo.EnableAutoExposure ? NVSDK_NGX_DLSS_Feature_Flags_AutoExposure : 0);

    auto dlssCreateParameters = NVSDK_NGX_DLSS_Create_Params();
    dlssCreateParameters.Feature.InWidth = initializeInfo.RenderResolution.Width;
    dlssCreateParameters.Feature.InHeight = initializeInfo.RenderResolution.Height;
    dlssCreateParameters.Feature.InTargetWidth = initializeInfo.OutputResolution.Width;
    dlssCreateParameters.Feature.InTargetHeight = initializeInfo.OutputResolution.Height;
    dlssCreateParameters.Feature.InPerfQualityValue = [&] -> NVSDK_NGX_PerfQuality_Value {
      switch (initializeInfo.Quality) {
        case ENvidiaDlssQualityPreset::E_PERFORMANCE: return NVSDK_NGX_PerfQuality_Value_MaxPerf;
        case ENvidiaDlssQualityPreset::E_BALANCED: return NVSDK_NGX_PerfQuality_Value_Balanced;
        case ENvidiaDlssQualityPreset::E_QUALITY: return NVSDK_NGX_PerfQuality_Value_MaxQuality;
        case ENvidiaDlssQualityPreset::E_NATIVE: return NVSDK_NGX_PerfQuality_Value_DLAA;
        default: std::unreachable();
      }
    }();
    dlssCreateParameters.InFeatureCreateFlags = dlssFlags;

    const auto preset = NVSDK_NGX_DLSS_Hint_Render_Preset_Default;
    NVSDK_NGX_Parameter_SetUI(_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_DLAA, preset);
    NVSDK_NGX_Parameter_SetUI(_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Quality, preset);
    NVSDK_NGX_Parameter_SetUI(_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Balanced, preset);
    NVSDK_NGX_Parameter_SetUI(_parameters, NVSDK_NGX_Parameter_DLSS_Hint_Render_Preset_Performance, preset);

    _device->GetGraphicsQueue().Submit([&](CCommandBuffer& commands) noexcept {
      RETINA_NVIDIA_CHECK(
        NGX_VULKAN_CREATE_DLSS_EXT(
          commands.GetHandle(),
          1,
          1,
          &_handle,
          _parameters,
          &dlssCreateParameters
        )
      );
    });
  }

  auto CNvidiaDlssFeature::Shutdown() noexcept -> void {
    RETINA_PROFILE_SCOPED();
    if (_handle) {
      NVSDK_NGX_VULKAN_ReleaseFeature(_handle);
      _handle = nullptr;
    }
  }

  auto CNvidiaDlssFeature::Evaluate(CCommandBuffer& commands, const SNvidiaDlssEvaluateInfo& evaluateInfo) noexcept -> void {
    RETINA_PROFILE_SCOPED();
    const auto width = evaluateInfo.Color->GetWidth();
    const auto height = evaluateInfo.Color->GetHeight();
    auto colorResource = Details::ToNvidiaResource(*evaluateInfo.Color);
    auto depthResource = Details::ToNvidiaResource(*evaluateInfo.Depth);
    auto velocityResource = Details::ToNvidiaResource(*evaluateInfo.Velocity);
    auto outputResource = Details::ToNvidiaResource(*evaluateInfo.Output);

    auto dlssEvaluateParameters = NVSDK_NGX_VK_DLSS_Eval_Params();
    dlssEvaluateParameters.Feature.pInColor = &colorResource;
    dlssEvaluateParameters.Feature.pInOutput = &outputResource;
    dlssEvaluateParameters.Feature.InSharpness = 0.0f;
    dlssEvaluateParameters.pInDepth = &depthResource;
    dlssEvaluateParameters.pInMotionVectors = &velocityResource;
    dlssEvaluateParameters.InJitterOffsetX = evaluateInfo.JitterOffset.x;
    dlssEvaluateParameters.InJitterOffsetY = evaluateInfo.JitterOffset.y;
    dlssEvaluateParameters.InRenderSubrectDimensions = { width, height };
    dlssEvaluateParameters.InReset = evaluateInfo.Reset;
    dlssEvaluateParameters.InMVScaleX = evaluateInfo.MotionVectorScale.x;
    dlssEvaluateParameters.InMVScaleY = evaluateInfo.MotionVectorScale.y;

    RETINA_NVIDIA_CHECK(
      NGX_VULKAN_EVALUATE_DLSS_EXT(
        commands.GetHandle(),
        _handle,
        _parameters,
        &dlssEvaluateParameters
      )
    );
  }

  auto GetNvidiaDlssIstanceExtensions() noexcept -> std::vector<const char*> {
    RETINA_PROFILE_SCOPED();
    auto count = 0_u32;
    auto* extensions = Core::Null<VkExtensionProperties>();
    RETINA_NVIDIA_CHECK(
      NVSDK_NGX_VULKAN_GetFeatureInstanceExtensionRequirements(
        &Details::NVIDIA_DLSS_FEATURE_DISCOVERY_INFO,
        &count,
        &extensions
      )
    );
    auto result = std::vector<const char*>();
    result.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      result.emplace_back(extensions[i].extensionName);
    }
    return result;
  }

  auto GetNvidiaDlssDeviceExtensions(VkInstance instance, VkPhysicalDevice physicalDevice) noexcept -> std::vector<const char*> {
    RETINA_PROFILE_SCOPED();
    auto count = 0_u32;
    auto* extensions = Core::Null<VkExtensionProperties>();
    RETINA_NVIDIA_CHECK(
      NVSDK_NGX_VULKAN_GetFeatureDeviceExtensionRequirements(
        instance,
        physicalDevice,
        &Details::NVIDIA_DLSS_FEATURE_DISCOVERY_INFO,
        &count,
        &extensions
      )
    );
    auto result = std::vector<const char*>();
    result.reserve(count);
    for (auto i = 0_u32; i < count; ++i) {
      result.emplace_back(extensions[i].extensionName);
    }
    return result;
  }
}
