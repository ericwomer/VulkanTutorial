#if !defined(VULKANFUNCTIONS_H)
#define VULKANFUNCTIONS_H

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#define VK_EXPORTED_FUNCTION(fun) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;

#include "VulkanFunctions.inl"

#endif  // VULKANFUNCTIONS_H
