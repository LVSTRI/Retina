#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Native/NativeDebugName.hpp>
#include <Retina/Graphics/QueryPoolInfo.hpp>

namespace Retina {
    class CQueryPool : public INativeDebugName, public IEnableIntrusiveReferenceCount<CQueryPool> {
    public:
        using Self = CQueryPool;

        CQueryPool() noexcept = default;
        ~CQueryPool() noexcept;

        RETINA_NODISCARD static auto Make(
            const CDevice& device,
            const SQueryPoolCreateInfo& createInfo
        ) noexcept -> CArcPtr<Self>;

        RETINA_NODISCARD auto GetHandle() const noexcept -> VkQueryPool;
        RETINA_NODISCARD auto GetType() const noexcept -> EQueryType;
        RETINA_NODISCARD auto GetCount() const noexcept -> uint32;
        RETINA_NODISCARD auto GetCreateInfo() const noexcept -> const SQueryPoolCreateInfo&;
        RETINA_NODISCARD auto GetDevice() const noexcept -> const CDevice&;

        auto SetDebugName(std::string_view name) noexcept -> void;

        template <typename T>
            requires (std::same_as<T, uint32> || std::same_as<T, uint64>)
        auto FetchResult(EQueryResultFlag flags) const noexcept -> std::optional<std::vector<T>>;

    private:
        auto FetchResult(EQueryResultFlag flags) const noexcept -> std::optional<std::vector<uint8>>;

        VkQueryPool _handle = {};

        SQueryPoolCreateInfo _createInfo = {};
        CArcPtr<const CDevice> _device;
    };

    template <typename T>
        requires (std::same_as<T, uint32> || std::same_as<T, uint64>)
    auto CQueryPool::FetchResult(EQueryResultFlag flags) const noexcept -> std::optional<std::vector<T>> {
        RETINA_PROFILE_SCOPED();
        if constexpr (sizeof(T) == sizeof(uint64)) {
            if (!IsFlagEnabled(flags, EQueryResultFlag::E_64_BIT)) {
                flags |= EQueryResultFlag::E_64_BIT;
            }
        }
        const auto isAvailabilityRequested = IsFlagEnabled(flags, EQueryResultFlag::E_WITH_AVAILABILITY);
        auto result = FetchResult(flags);
        if (!result) {
            return std::nullopt;
        }
        auto values = std::vector<T>(result->size() / sizeof(T));
        std::memcpy(values.data(), result->data(), result->size());
        if (isAvailabilityRequested) {
            for (auto i = 0_u32; i < GetCount(); ++i) {
                if (values[GetCount() * 2 + i] == 0) {
                    return std::nullopt;
                }
            }
        }
        return values;
    }
}
