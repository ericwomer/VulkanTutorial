#include <vector>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include "VulkanFactories.h"
#include "VulkanFunctions.h"

namespace Rake::Graphics::Factory {
/**
 * @brief
 *
 * @param code
 * @return VkShaderModule
 */
VkShaderModule Shader::create_shader_module(VkDevice device, const std::vector<char> code)
{
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize                 = code.size();
  createInfo.pCode                    = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;

  if (auto result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule); result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create shader module!");
  }
  return shaderModule;
}

}  // namespace Rake::Graphics::Factory
