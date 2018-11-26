#include "VulkanFunctions.h"

#include <vulkan/vulkan.h>

namespace Rake { namespace Application {
#define VK_EXPORTED_FUNCTION(fun) PFN_##fun fun;
#define VK_GLOBAL_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_INSTANCE_LEVEL_FUNCTION(fun) PFN_##fun fun;
#define VK_DEVICE_LEVEL_FUNCTION(fun) PFN_##fun fun;
#include "VulkanFunctions.inl"
}}  // namespace Rake::Application
