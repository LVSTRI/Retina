#include <Retina/Graphics/Device.hpp>
#include <Retina/Graphics/Pipeline.hpp>

#include <volk.h>

#if defined(_WIN32)
    #include <atlbase.h>
    #include <Windows.h>
#endif
#include <dxc/dxcapi.h>

#include <vector>
#include <ranges>
#include <format>
#include <span>

#define RETINA_CHECK_D3D_RESULT(result) RETINA_ASSERT_WITH(SUCCEEDED(result), "Fatal D3D Error")

namespace Retina {
    namespace Private {
        static struct SShaderCompiler {
            SShaderCompiler() noexcept {
                RETINA_PROFILE_SCOPED();
                RETINA_CHECK_D3D_RESULT(DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library)));
                RETINA_CHECK_D3D_RESULT(DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
                RETINA_CHECK_D3D_RESULT(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
                RETINA_CHECK_D3D_RESULT(library->CreateIncludeHandler(&includer));
            }

            ~SShaderCompiler() noexcept = default;

            CComPtr<IDxcLibrary> library;
            CComPtr<IDxcCompiler3> compiler;
            CComPtr<IDxcUtils> utils;
            CComPtr<IDxcIncludeHandler> includer;
        } shaderCompiler = {};
    }

    auto CompileShaderFromSource(
        const CDevice& device,
        const fs::path& path,
        std::span<const fs::path> includeDirectories,
        std::wstring_view entryPoint,
        EShaderStage shaderStage
    ) noexcept -> std::vector<uint32> {
        RETINA_PROFILE_SCOPED();
        const auto rootShaderPath = fs::path(RETINA_MAIN_SHADER_PATH);
        const auto& [library, compiler, utils, includer] = Private::shaderCompiler;

        auto encoding = uint32(DXC_CP_ACP);
        auto sourceBlob = CComPtr<IDxcBlobEncoding>();
        RETINA_CHECK_D3D_RESULT(utils->LoadFile((rootShaderPath / path).wstring().c_str(), &encoding, &sourceBlob));
        const auto* targetProfile = [&] {
            switch (shaderStage) {
                case EShaderStage::E_VERTEX: return L"vs";
                case EShaderStage::E_TESSELLATION_CONTROL: return L"hs";
                case EShaderStage::E_TESSELLATION_EVALUATION: return L"ds";
                case EShaderStage::E_GEOMETRY: return L"gs";
                case EShaderStage::E_FRAGMENT: return L"ps";
                case EShaderStage::E_COMPUTE: return L"cs";
                case EShaderStage::E_RAYGEN_KHR: RETINA_FALLTHROUGH;
                case EShaderStage::E_ANY_HIT_KHR: RETINA_FALLTHROUGH;
                case EShaderStage::E_CLOSEST_HIT_KHR:  RETINA_FALLTHROUGH;
                case EShaderStage::E_MISS_KHR: RETINA_FALLTHROUGH;
                case EShaderStage::E_INTERSECTION_KHR: RETINA_FALLTHROUGH;
                case EShaderStage::E_CALLABLE_KHR: return L"lib";
                case EShaderStage::E_TASK_EXT: return L"as";
                case EShaderStage::E_MESH_EXT: return L"ms";
                default: RETINA_PANIC();
            }
        }();

        auto arguments = std::vector<std::wstring> {
            L"-E", std::wstring(entryPoint),
            L"-T", std::format(L"{}_6_6", targetProfile),
            L"-spirv",
            L"-fspv-target-env=vulkan1.3",
            L"-fvk-use-gl-layout",
            L"-Wno-conversion",
            L"-Zpc",
            L"-Zi",
            L"-O0",
        };
        const auto appendArguments = [&](std::span<const std::wstring> data) {
            arguments.insert(arguments.end(), data.begin(), data.end());
        };
        for (const auto& includeDirectory : includeDirectories) {
            appendArguments(std::to_array({
                L"-I"s, (rootShaderPath / includeDirectory).wstring()
            }));
        }
        appendArguments(std::to_array({
            L"-I"s, rootShaderPath.wstring(),
        }));
        auto argumentsPtrs = std::vector<const wchar_t*>();
        argumentsPtrs.reserve(arguments.size());
        for (const auto& argument : arguments) {
            argumentsPtrs.emplace_back(argument.c_str());
        }

        auto buffer = DxcBuffer();
        buffer.Encoding = DXC_CP_ACP;
        buffer.Ptr = sourceBlob->GetBufferPointer();
        buffer.Size = sourceBlob->GetBufferSize();
        auto compileResult = CComPtr<IDxcResult>(nullptr);
        RETINA_CHECK_D3D_RESULT(
            compiler->Compile(
                &buffer,
                argumentsPtrs.data(),
                static_cast<uint32>(argumentsPtrs.size()),
                includer,
                IID_PPV_ARGS(&compileResult)
            )
        );
        if (compileResult) {
            auto errorBuffer = CComPtr<IDxcBlobEncoding>();
            RETINA_CHECK_D3D_RESULT(compileResult->GetErrorBuffer(&errorBuffer));
            if (errorBuffer->GetBufferSize() > 0) {
                RETINA_PANIC_WITH(
                    device.GetLogger(),
                    "DXC Shader Compile Failure Log for Shader \"{}\": \"{}\"",
                    path.string(),
                    static_cast<const char*>(errorBuffer->GetBufferPointer())
                );
            }

        }
        auto spirvBlob = CComPtr<IDxcBlob>();
        RETINA_CHECK_D3D_RESULT(compileResult->GetResult(&spirvBlob));
        const auto* spirvPtr = static_cast<const uint32*>(spirvBlob->GetBufferPointer());
        const auto spirvSize = spirvBlob->GetBufferSize() / sizeof(uint32);
        return { spirvPtr, spirvPtr + spirvSize };
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
