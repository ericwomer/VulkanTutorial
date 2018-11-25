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

#include <ostream>
#include <fstream>

#include <vulkan/vulkan.h>
// #include <SDL.h>

#include "skeleton/skeleton.h"
#include "vulkan/VulkanFunctions.h"

// Temp define
#define VK_USE_PLATFORM_XCB_KHR

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <xcb/xcb.h>
#include <dlfcn.h>
#endif

namespace rake { namespace vlkn {

constexpr int global_width  = 640;
constexpr int global_height = 480;

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

class vkTutorialApp : public Skeleton {
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
  uint32_t width  = global_width;
  uint32_t height = global_height;

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
  VkPipelineLayout             pipelineLayout;
  VkPipeline                   graphicsPipeline;
  std::vector<VkFramebuffer>   swapchainFramebuffers;
  VkCommandPool                commandPool;
  std::vector<VkCommandBuffer> commandBuffers;

  std::vector<VkSemaphore> imageAvailableSemaphore;
  std::vector<VkSemaphore> renderFinishedSemaphore;
  std::vector<VkFence>     inFlightFences;
  bool                     framebufferResized = false;

  void*             VulkanLibrary = nullptr;
  xcb_connection_t* connection;
  xcb_window_t      handle;
  bool              canRender;

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

  // Vulkan Private Interface Methods.
  void init_window();
  void init_vulkan();
  void init_input();
  void setup_debug_callback();
  bool draw();  // Vulkan-tutorial.com DrawFrame

  std::vector<const char*> get_required_extensions();
  SwapChainSupportDetails  query_swap_chain_support(VkPhysicalDevice device);
  QueueFamilyIndices       find_queue_families(VkPhysicalDevice device);
  bool                     check_validation_layer_support();
  bool                     check_device_extension_support(VkPhysicalDevice device);
  bool                     is_device_suitable(VkPhysicalDevice device);
  void                     get_device_queues();
  VkSurfaceFormatKHR       choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
  VkPresentModeKHR         choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes);
  VkExtent2D               choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

  // Initialization
  void create_instance();
  void pick_physical_device();
  void create_logical_device();
  void create_surface();
  void create_swap_chain();
  void create_image_views();
  void create_render_pass();
  void create_graphics_pipeline();
  void create_frame_buffer();
  void create_command_pool();
  void create_command_buffers();
  void create_sync_objects();

  // Recreation
  void recreate_swap_chain();

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
};
}}      // namespace rake::vlkn
#endif  // SKELETONAPP_H
