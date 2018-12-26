#include <vulkan/vulkan.h>

#include <vector>

#include <vulkan/vulkan.h>

namespace Rake::Graphics::Factory {

class Shader {
  public:
  VkShaderModule create_shader_module(VkDevice device, const std::vector<char> code);
};

}  // namespace Rake::Graphics::Factory
