#pragma once

#include <Retina/Core/Core.hpp>

namespace Retina {
    struct SQueryPoolCreateInfo {
        std::string Name;
        EQueryType Type = {};
        uint32 Count = 0;
        EQueryPipelineStatistic PipelineStatisticFlags = {};
    };
}
