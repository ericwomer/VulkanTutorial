#include <unordered_map>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include "VulkanFunctions.h"

#include "VulkanCore.h"
#include "VulkanObjects.h"
#include "VulkanUtilities.h"

namespace std {
template <>
struct hash<Rake::Graphics::Object::Vertex> {
  size_t operator()(Rake::Graphics::Object::Vertex const& vertex) const
  {
    return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^
           (hash<glm::vec2>()(vertex.texCoord) << 1);
  }
};
}  // namespace std

namespace Rake { namespace Graphics {

/**
 * @brief
 */
void Utility::load_model(Object::Model& model)
{
  tinyobj::attrib_t                attrib;
  std::vector<tinyobj::shape_t>    shapes;
  std::vector<tinyobj::material_t> materials;
  std::string                      warn, err;

  if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model.modelPath.c_str())) {
    throw std::runtime_error(warn + err);
  }

  std::unordered_map<Object::Vertex, uint32_t> uniqueVertices = {};

  for (const auto& shape : shapes) {
    for (const auto& index : shape.mesh.indices) {
      Object::Vertex vertex = {};

      vertex.pos = {attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]};

      vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
                         1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

      vertex.color = {1.0f, 1.0f, 1.0f};

      if (uniqueVertices.count(vertex) == 0) {
        uniqueVertices[vertex] = static_cast<uint32_t>(model.verticies.size());
        model.verticies.push_back(vertex);
      }

      model.indices.push_back(uniqueVertices[vertex]);
    }
  }
}

/**
 * @brief
 *
 * @param filename
 * @return std::vector<char>
 */
std::vector<char> Utility::read_file(const std::string& filename)
{
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("Failed to open file!");
  }

  size_t            fileSize = (size_t)file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

// Helpers

/**
 * @brief
 * @param candidates
 * @param tiling
 * @param features
 * @return
 */
VkFormat Helper::find_supported_format(VkPhysicalDevice&            physicalDevice,
                                       const std::vector<VkFormat>& candidates,
                                       const VkImageTiling&         tiling,
                                       const VkFormatFeatureFlags&  features)
{
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
      return format;
    } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
      return format;
    }
  }
  throw std::runtime_error("Failed to find supported format!");
}

/**
 * @brief
 * @return
 */
VkFormat Helper::find_depth_format(VkPhysicalDevice& physicalDevice)
{
  std::vector<VkFormat> candidates = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
  return find_supported_format(physicalDevice,
                               candidates,
                               VK_IMAGE_TILING_OPTIMAL,
                               VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

/**
 * @brief
 * @param format
 * @return
 */
bool Helper::has_stencil_component(VkFormat& format)
{
  return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/**
* @brief Get the VkExtensionProperties object and fill extensionNames and
* extensionCount
*
*/
std::vector<const char*> Helper::get_required_extensions(const std::vector<const char*>& instanceExtensions)
{
  uint32_t                           instanceExtensionCount = 0;
  std::vector<VkExtensionProperties> vkInstanceExtensions;
  vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
  vkInstanceExtensions.resize(instanceExtensionCount);
  vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, vkInstanceExtensions.data());

  std::cout << "\nNumber of availiable instance extensions\t" << vkInstanceExtensions.size() << "\n";
  std::cout << "Available Extension List: \n";
  for (auto& ext : vkInstanceExtensions) {
    std::cout << "\t" << ext.extensionName << "\n";
  }

  std::vector<const char*> extensions;
  for (auto& ext : instanceExtensions) {
    extensions.push_back(ext);
  }

  if (enableValidationLayers) {
    extensions.push_back("VK_EXT_debug_report");
    extensions.push_back("VK_EXT_debug_utils");
  }

  std::cout << "Number of active instance extensions\t" << extensions.size() << "\n";
  std::cout << "Active Extension List: \n";
  for (auto const& ext : extensions) {
    std::cout << "\t" << ext << "\n";
  }

  return extensions;
}

/**
* @brief Check for Validation Layer support
*
* @return true
* @return false
*/
bool Helper::check_validation_layer_support(const std::vector<const char*>& validationLayers)
{
  uint32_t                 layerCount = 0;
  std::vector<std::string> activeLayers;

  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  std::cout << "Number of available validation layers " << layerCount << "\n";
  std::cout << "Available Layers: \n";
  for (const char* layerName : validationLayers) {
    bool layerFound = false;
    activeLayers.push_back(layerName);
    for (auto& layerProperties : availableLayers) {
      std::cout << "\t" << layerProperties.layerName << "\n";
      if (strcmp(layerName, layerProperties.layerName) == 0) {
        layerFound = true;
        break;
      }
    }
    if (!layerFound) {
      return false;
    }
  }
  std::cout << "Number of active layers " << activeLayers.size() << "\n";
  std::cout << "Active Layers: \n";
  for (const auto& activeLayer : activeLayers) {
    std::cout << "\t" << activeLayer << "\n";
  }

  return true;
}

/**
* @brief
*
* @param device
* @return true
* @return false
*/
bool Helper::is_device_suitable(VkPhysicalDevice device, VkSurfaceKHR surface)
{
  Core::QueueFamilyIndices indicies           = find_queue_families(device, surface);
  bool                     extensionSupported = check_device_extension_support(device);
  bool                     swapChainAdequate  = false;

  if (extensionSupported) {
    Core::SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device, surface);
    swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
  }

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

  return indicies.is_complete() && extensionSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
}

/**
* @brief
*
* @param device
* @return true
* @return false
*/
bool Helper::check_device_extension_support(VkPhysicalDevice device)
{
  uint32_t extensionCount = 0;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto& extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

/**
 * @brief
 * @param device
 * @return
 */
Core::SwapChainSupportDetails Helper::query_swap_chain_support(VkPhysicalDevice& device, VkSurfaceKHR& surface)
{
  Core::SwapChainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

  uint32_t formatCount = 0;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount = 0;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
  }

  return details;
}

/**
* @brief
*
* @param device
* @return Core::QueueFamilyIndices
*/
Core::QueueFamilyIndices Helper::find_queue_families(VkPhysicalDevice& device, VkSurfaceKHR& surface)
{
  Core::QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  int i = 0;
  for (const auto& queueFamily : queueFamilies) {
    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
    if (queueFamily.queueCount > 0 && presentSupport) {
      indices.presentFamily = i;
    }

    if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      indices.graphicsFamily = i;
    }

    if (indices.is_complete()) {
      break;
    }
    i++;
  }
  return indices;
}

/**
 * @brief
 *
 */
void Helper::get_device_queues(VkDevice&                 device,
                               Core::QueueFamilyIndices& familyIndicies,
                               VkQueue&                  presentQueue,
                               VkQueue&                  graphicsQueue)
{
  vkGetDeviceQueue(device, familyIndicies.presentFamily.value(), 0, &presentQueue);
  vkGetDeviceQueue(device, familyIndicies.graphicsFamily.value(), 0, &graphicsQueue);
}

/**
 * @brief
 * @param typeFilter
 * @param properties
 * @return
 */
uint32_t Helper::find_memory_type(VkPhysicalDevice&      physicalDevice,
                                  uint32_t&              typeFilter,
                                  VkMemoryPropertyFlags& properties)
{
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }
  throw std::runtime_error("Failed to suitable memory type!");
}

/**
 * @brief
 * @return
 */
VkSampleCountFlagBits Helper::get_max_usable_sample_count(VkPhysicalDevice& physicalDevice)
{
  VkPhysicalDeviceProperties physicalDeviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

  VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts,
                                       physicalDeviceProperties.limits.framebufferDepthSampleCounts);

  if (counts & VK_SAMPLE_COUNT_64_BIT) {
    return VK_SAMPLE_COUNT_64_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_32_BIT) {
    return VK_SAMPLE_COUNT_32_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_16_BIT) {
    return VK_SAMPLE_COUNT_16_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_8_BIT) {
    return VK_SAMPLE_COUNT_8_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_4_BIT) {
    return VK_SAMPLE_COUNT_4_BIT;
  }
  if (counts & VK_SAMPLE_COUNT_2_BIT) {
    return VK_SAMPLE_COUNT_2_BIT;
  }
  return VK_SAMPLE_COUNT_1_BIT;
}
}}  // namespace Rake::Graphics
