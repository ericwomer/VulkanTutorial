#if !defined(SKELETONAPP_H)
#define SKELETONAPP_H

#include <cstdlib>
#include <cstring>
#include <cstdint>

#include <functional>
#include <algorithm>
#include <optional>
#include <set>

#include <memory>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <array>

#include <ostream>
#include <fstream>

#include <vulkan/vulkan.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

// #include <SDL.h>

#include "skeleton/skeleton.h"
#include "vulkan/VulkanFunctions.h"

// Temp define
#define VK_USE_PLATFORM_XCB_KHR

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <dlfcn.h>
#endif

// clang-format off
// clang-format on

namespace Rake { namespace Application {

struct UniformBufferObject {
  glm::mat4 model;
  glm::mat4 view;
  glm::mat4 proj;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 color;
  glm::vec2 texCoord;

  static VkVertexInputBindingDescription getBindingDescription()
  {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding                         = 0;
    bindingDescription.stride                          = sizeof(Vertex);
    bindingDescription.inputRate                       = VK_VERTEX_INPUT_RATE_VERTEX;

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 3> getAttributesDescription()
  {
    std::array<VkVertexInputAttributeDescription, 3> attributesDescription = {};

    attributesDescription[0].binding  = 0;
    attributesDescription[0].location = 0;
    attributesDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributesDescription[0].offset   = offsetof(Vertex, pos);

    attributesDescription[1].binding  = 0;
    attributesDescription[1].location = 1;
    attributesDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    attributesDescription[1].offset   = offsetof(Vertex, color);

    attributesDescription[2].binding  = 0;
    attributesDescription[2].location = 2;
    attributesDescription[2].format   = VK_FORMAT_R32G32_SFLOAT;
    attributesDescription[2].offset   = offsetof(Vertex, texCoord);

    return attributesDescription;
  }

  bool operator==(const Vertex& other) const
  {
    return pos == other.pos && color == other.color && texCoord == other.texCoord;
  }
};

struct Model {
  int width;   // = 800;
  int height;  // = 600;

  std::string modelPath;    // = "data/models/chalet.obj"
  std::string texturePath;  // = "data/textures/chalet.jpg"

  std::vector<Vertex>   verticies;
  std::vector<uint32_t> indices;
  VkBuffer              vertexBuffer;
  VkDeviceMemory        vertexBufferMemory;
};

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

#if defined(NDEBUG)
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult create_debug_util_messenger_ext(VkInstance                                instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks*              pAllocator,
                                         VkDebugUtilsMessengerEXT*                 pCallback);

void destroy_debug_utils_messenger_ext(VkInstance                   instance,
                                       VkDebugUtilsMessengerEXT     callback,
                                       const VkAllocationCallbacks* pAllocator);

class vkTutorialApp : public Rake::Base::Skeleton {
  public:
  // Core Application Public Interface Methods
  vkTutorialApp();
  virtual ~vkTutorialApp(){};
  virtual int               main() override;
  virtual int               main(std::vector<std::string>& params) override;
  virtual int               size() override { return sizeof(this); };
  virtual const std::string name() override { return app_name; };
  virtual void              help() override;
  virtual std::string       name() const override { return app_name; };
  virtual void              name(const std::string& name) override { app_name = name; };
  virtual void              version() const override;
  virtual void              version(const Version_t& version) override { version_number = version; };

  // Vulkan Public Interface Methods.
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT             messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                       void*                                       pUserData)
  {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
  }

  private:
  // Core Application Private Member Variables
  std::vector<std::string> actions;

  // Vulkan Private Member Variables
  uint32_t width  = 640;
  uint32_t height = 480;

  const int MAX_FRAMES_IN_FLIGHT = 2;
  size_t    currentFrame         = 0;

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

  uint32_t       mipLevels;
  VkImage        textureImage;
  VkDeviceMemory textureImageMemory;
  VkImageView    textureImageView;
  VkSampler      textureSampler;

  VkImage        depthImage;
  VkDeviceMemory depthImageMemory;
  VkImageView    depthImageView;

  std::vector<VkSemaphore> imageAvailableSemaphore;
  std::vector<VkSemaphore> renderFinishedSemaphore;
  std::vector<VkFence>     inFlightFences;
  bool                     framebufferResized = false;

  void*             VulkanLibrary = nullptr;
  xcb_connection_t* connection    = nullptr;
  xcb_window_t      handle        = 0;
  bool              canRender     = false;

  // SDL_Window* window;
  void*    window;                    // Eric: void for now until I get the windowing part up and runnint
  char*    extensionStringNames[64];  // Eric: why am I char const *?
  uint32_t extensionCount = 0;

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

  QueueFamilyIndices familyIndicies;

  Model chalet;

  // Vulkan Private Interface Methods.
  void init_window();
  void init_vulkan();
  void init_input();
  void setup_debug_callback();
  bool draw();  // Vulkan-tutorial.com DrawFrame
  void update_uniform_buffer(uint32_t currentImage);

  std::vector<const char*> get_required_extensions();
  SwapChainSupportDetails  query_swap_chain_support(VkPhysicalDevice device);
  QueueFamilyIndices       find_queue_families(VkPhysicalDevice device);
  bool                     check_validation_layer_support();
  bool                     check_device_extension_support(VkPhysicalDevice device);
  bool                     is_device_suitable(VkPhysicalDevice device);
  void                     get_device_queues();
  uint32_t                 find_memory_type(uint32_t typeFilter, VkMemoryPropertyFlags properties);
  VkFormat                 find_supported_format(const std::vector<VkFormat>& candidates,
                                                 VkImageTiling                tiling,
                                                 VkFormatFeatureFlags         features);
  VkFormat                 find_depth_format();
  bool                     has_stencil_component(VkFormat format);

  void create_buffer(VkDeviceSize          size,
                     VkBufferUsageFlags    usage,
                     VkMemoryPropertyFlags properties,
                     VkBuffer&             buffer,
                     VkDeviceMemory&       bufferMemory);

  void            create_image(uint32_t              width,
                               uint32_t              height,
                               uint32_t              mipLevels,
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

  VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR   choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes);
  VkExtent2D         choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

  // Initialization
  void create_instance();
  void pick_physical_device();
  void create_logical_device();
  void create_surface();
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
  void cleanup();
  void cleanup_swapchain();

  static std::vector<char> read_file(const std::string& filename);
  VkShaderModule           create_shader_module(const std::vector<char>& code);

  // Vulkan Private Interface Methods Borrowed from https://software.intel.com/en-us/articles/api-without-secrets-introduction-to-vulkan-part-1
  bool ready_to_draw() { return canRender; }
  bool rendering_loop();
  bool on_window_size_changed();
  bool on_child_window_size_changed();
  bool clear();
  bool load_vulkan_library();
  bool load_exported_entry_points();
  bool load_global_entry_points();
  bool load_instance_level_entry_points();
  bool load_device_entry_level_points();

  // Misc Private Methods
  void load_model();
};
}}      // namespace Rake::Application
#endif  // SKELETONAPP_H
