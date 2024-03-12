#pragma once

#include <Retina/Core/Core.hpp>

#include <Retina/Graphics/Forward.hpp>

namespace Retina::Graphics {
  struct SDeletionQueuePacket {
    uint64 TimelineValue = 0;
    std::move_only_function<void()> Deletion;
  };
}
