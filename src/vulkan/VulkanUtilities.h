#if !defined(VULKANUTILITIES_H)
#define VULKANUTILITIES_H

#include <vulkan/vulkan.h>

#include "VulkanCore.h"

namespace Rake { namespace Graphics {

class Helper {
  public:
  std::vector<const char*>      get_required_extensions(const std::vector<const char*>& instanceExtensions);
  Core::SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice& device, VkSurfaceKHR& surface);
  Core::QueueFamilyIndices      find_queue_families(VkPhysicalDevice& device, VkSurfaceKHR& surface);
  bool                          check_validation_layer_support(const std::vector<const char*>& validationLayers);
  bool                          check_device_extension_support(VkPhysicalDevice device);
  bool                          is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface);
  void                          get_device_queues(VkDevice&                 device,
                                                  Core::QueueFamilyIndices& familyIndicies,
                                                  VkQueue&                  presentQueue,
                                                  VkQueue&                  graphicsQueue);
  uint32_t find_memory_type(VkPhysicalDevice& physicalDevice, uint32_t& typeFilter, VkMemoryPropertyFlags& properties);
  VkFormat find_supported_format(VkPhysicalDevice&            device,
                                 const std::vector<VkFormat>& candidates,
                                 const VkImageTiling&         tiling,
                                 const VkFormatFeatureFlags&  features);
  VkFormat find_depth_format(VkPhysicalDevice& physicalDevice);
  bool     has_stencil_component(VkFormat& format);
  VkSampleCountFlagBits get_max_usable_sample_count(VkPhysicalDevice& physicalDevice);
};

class Utility {
  public:
  void                     load_model(Object::Model& model);
  static std::vector<char> read_file(const std::string& filename);
};

}}  // namespace Rake::Graphics

#endif  // VULKANUTILITIES_H
