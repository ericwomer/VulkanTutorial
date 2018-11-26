#if !defined(VULKANFUNCTIONS_H)
#define VULKANFUNCTIONS_H

#include <vulkan/vulkan.h>

namespace Rake { namespace Application {
#define VK_EXPORTED_FUNCTION(fun) extern PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) extern PFN_##fun fun;

#include "VulkanFunctions.inl"
}}      // namespace Rake::Application
#endif  // VULKANFUNCTIONS_H
