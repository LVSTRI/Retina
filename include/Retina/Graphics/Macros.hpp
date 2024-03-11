#pragma once

#include <Retina/Core/Macros.hpp>

#include <Retina/Graphics/Logger.hpp>

#include <vulkan/vk_enum_string_helper.h>

#define RETINA_GRAPHICS_VULKAN_CHECK(expr)                      \
  do {                                                          \
    auto _result = (expr);                                      \
    if (_result != VK_SUCCESS) {                                \
      RETINA_GRAPHICS_CRITICAL("{}", string_VkResult(_result)); \
      RETINA_PANIC();                                           \
    }                                                           \
  } while (false)


#define RETINA_GRAPHICS_DEBUG_NAME(device, object, type, name)  \
  do {                                                          \
    if (name.empty()) {                                         \
      return;                                                   \
    }                                                           \
    auto nameInfo = VkDebugUtilsObjectNameInfoEXT(              \
      VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT        \
    );                                                          \
    nameInfo.objectHandle = (uint64)object;                     \
    nameInfo.objectType = type;                                 \
    nameInfo.pObjectName = name.data();                         \
    vkSetDebugUtilsObjectNameEXT(device, &nameInfo);            \
  } while (false)
