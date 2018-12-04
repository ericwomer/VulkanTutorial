#if !defined(VULKANCOMMON_H)
#define VULKANCOMMON_H

#include <functional>
#include <algorithm>
#include <optional>
#include <set>

#include <memory>
#include <stdexcept>

#include <iostream>
#include <string>
#include <vector>
#include <array>

#include <ostream>
#include <fstream>

#define VK_NO_PROTOTYPES
#define VK_USE_PLATFORM_XCB_KHR
#include <vulkan/vulkan.h>

#include "VulkanFunctions.h"
#include "VulkanObjects.h"
#include "VulkanFactories.h"
// #include "VulkanUtilities.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <dlfcn.h>
#endif

namespace Rake { namespace Graphics {
#if defined(NDEBUG)
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Helper;
class Filesystem;

/**
 * @brief Static list of application validation layers for Vulkan debugging
 *
 */
const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};

/*
 * @brief Default instance extensions
 */
const std::vector<const char*> instanceExtensions = {"VK_KHR_xcb_surface", "VK_KHR_surface"};

/*
 * @brief  Default device extensions
 */
const std::vector<const char*> deviceExtensions = {"VK_KHR_swapchain"};

VkResult create_debug_util_messenger_ext(VkInstance                                instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks*              pAllocator,
                                         VkDebugUtilsMessengerEXT*                 pCallback);

void destroy_debug_utils_messenger_ext(VkInstance                   instance,
                                       VkDebugUtilsMessengerEXT     callback,
                                       const VkAllocationCallbacks* pAllocator);

template <typename T>
std::unique_ptr<T> generate_unique_ptr();

class Core {
  public:
  Core();
  ~Core();

  // Vulkan Public Interface Methods.
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void*                                       pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  void init_vulkan(xcb_connection_t* connection, xcb_window_t handle);
  bool draw();  // Vulkan-tutorial.com DrawFrame
  void cleanup();
  bool ready_to_draw() { return canRender; }
  bool on_window_size_changed();

  struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool is_complete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
  };

  struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
  };

  private:
  int width  = 800;
  int height = 600;

  const int MAX_FRAMES_IN_FLIGHT = 2;
  size_t    currentFrame         = 0;

  std::unique_ptr<Helper>     helper;
  std::unique_ptr<Filesystem> filesystem;

  VkInstance                   instance;
  VkDebugUtilsMessengerEXT     callback;
  VkPhysicalDevice             physicalDevice = VK_NULL_HANDLE;
  VkDevice                     device;
  VkQueue                      graphicsQueue;
  VkSurfaceKHR                 surface;
  VkQueue                      presentQueue;
  VkSwapchainKHR               swapchain = VK_NULL_HANDLE;
  std::vector<VkImage>         swapchainImages;
  VkFormat                     swapchainImageFormat;
  VkExtent2D                   swapchainExtent;
  std::vector<VkImageView>     swapchainImageViews;
  VkRenderPass                 renderPass;
  VkDescriptorSetLayout        descriptorSetLayout;
  VkPipelineLayout             pipelineLayout;
  VkPipeline                   graphicsPipeline;
  std::vector<VkFramebuffer>   swapchainFramebuffers;
  VkCommandPool                commandPool;
  std::vector<VkCommandBuffer> commandBuffers;
  VkBuffer                     vertexBuffer = VK_NULL_HANDLE;
  VkDeviceMemory               vertexBufferMemory;
  VkBuffer                     indexBuffer;
  VkDeviceMemory               indexBufferMemory;
  std::vector<VkBuffer>        uniformBuffers;
  std::vector<VkDeviceMemory>  uniformBuffersMemory;
  VkDescriptorPool             descriptorPool;
  std::vector<VkDescriptorSet> descriptorSets;
  VkSampleCountFlagBits        msaaSamples = VK_SAMPLE_COUNT_1_BIT;

  uint32_t       mipLevels;
  VkImage        textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView    textureImageView;
  VkSampler      textureSampler;

  VkImage        depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView    depthImageView;

  VkImage        colorImage;
  VkDeviceMemory colorImageMemory;
  VkImageView    colorImageView;

  std::vector<VkSemaphore> imageAvailableSemaphore;
  std::vector<VkSemaphore> renderFinishedSemaphore;
  std::vector<VkFence>     inFlightFences;
  bool                     framebufferResized = false;

  VkShaderModule create_shader_module(const VkDevice& device, const std::vector<char>& code);

  void* VulkanLibrary = nullptr;
  bool  canRender     = false;

  char*    extensionStringNames[64];  // Eric: why am I char const *?
  uint32_t extensionCount = 0;

  QueueFamilyIndices familyIndicies;

  Object::Model chalet;

  // Vulkan Private Interface Methods.

  void setup_debug_callback();
  void update_uniform_buffer(uint32_t currentImage);
  bool clear();

  void create_buffer(VkDeviceSize          size,
                     VkBufferUsageFlags    usage,
                     VkMemoryPropertyFlags properties,
                     VkBuffer&             buffer,
                     VkDeviceMemory&       bufferMemory);

  void            create_image(uint32_t              width,
                               uint32_t              height,
                               uint32_t              mipLevels,
                               VkSampleCountFlagBits numSamples,
                               VkFormat              format,
                               VkImageTiling         tiling,
                               VkImageUsageFlags     usage,
                               VkMemoryPropertyFlags properties,
                               VkImage&              image,
                               VkDeviceMemory&       imageMemory);
  void            copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  VkCommandBuffer begin_single_time_commands();
  void            end_single_time_commands(VkCommandBuffer commandBuffer);
  void            transition_image_layout(VkImage       image,
                                          VkFormat      format,
                                          VkImageLayout oldLayout,
                                          VkImageLayout newLayout,
                                          uint32_t      mipLevels);
  void            copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
  VkImageView     create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
  void generate_mipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
  void create_color_resources();

  VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR   choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes);
  VkExtent2D         choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

  // Initialization
  void create_instance();
  void pick_physical_device();
  void create_logical_device();
  void create_surface(xcb_connection_t* connection, xcb_window_t handle);
  void create_swap_chain();
  void create_image_views();
  void create_render_pass();
  void create_descriptor_set_layout();
  void create_graphics_pipeline();
  void create_frame_buffer();
  void create_command_pool();
  void create_depth_resources();
  void create_command_buffers();
  void create_sync_objects();
  void create_texture_image();
  void create_texture_image_view();
  void create_texture_sampler();
  void create_vertex_buffer();
  void create_index_buffer();
  void create_uniform_buffers();
  void create_descriptor_pool();
  void create_descriptor_sets();

  // Recreation
  bool recreate_swap_chain();

  // Cleanup

  void cleanup_swapchain();

  // Vulkan Private Interface Methods Borrowed from https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1

  bool on_child_window_size_changed();

  bool load_vulkan_library();
  bool load_exported_entry_points();
  bool load_global_entry_points();
  bool load_instance_level_entry_points();
  bool load_device_entry_level_points();
};

}}  // namespace Rake::Graphics

#endif  // VULKANCOMMON_H
