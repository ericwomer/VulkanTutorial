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

constexpr int global_width = 1680;
constexpr int global_height = 1050;

namespace rake { namespace vlkn {
/**
 * @brief Static list of application validation layers for Vulkan debugging
 *
 */
const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};

// Default instance extensions
const std::vector<const char*> instanceExtensions = {"VK_KHR_xcb_surface", "VK_KHR_surface"};

// Default device extensions
const std::vector<const char*> deviceExtensions = {"VK_KHR_swapchain"};

#if defined(NDEBUG)
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

VkResult create_debug_util_messenger_ext(VkInstance instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkDebugUtilsMessengerEXT* pCallback);

void destroy_debug_utils_messenger_ext(VkInstance instance,
                                       VkDebugUtilsMessengerEXT callback,
                                       const VkAllocationCallbacks* pAllocator);

class vkTutorialApp : public Skeleton {
public:
    vkTutorialApp();
    virtual ~vkTutorialApp(){};

    virtual int main() override;
    virtual int main(int argv, char* argc[]) override;
    virtual int main(std::vector<std::string>& params) override;
    virtual int size() override { return sizeof(this); };
    virtual const std::string name() override { return app_name; };
    virtual void help() override;
    virtual std::string name() const override { return app_name; };
    virtual void name(const std::string& name) override { app_name = name; };
    virtual Version_t version() const override { return version_number; }
    virtual void version(const Version_t& version) override { version_number = version; };

    // Vulkan Interface Starts here.
    /**
	 * @brief My DebugCallback method
	 *
	 * @param messageSeverity The severity of the message
	 * @param messageType The type of message
	 * @param pCallbackData Any callback data
	 * @param pUserData You can pass user data to the callback, like the
	 * HelloTriangle App class
	 */
    static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                         const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                         void* pUserData)
    {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

private:
    std::vector<std::string> actions;

    // Vk Interface
    uint32_t width = global_width;
    uint32_t height = global_height;

    VkInstance instance;
    VkDebugUtilsMessengerEXT callback;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;
    VkSwapchainKHR swapchain;
    std::vector<VkImage> swapchainImages;
    VkFormat swapchainImageFormat;
    VkExtent2D swapchainExtent;
    std::vector<VkImageView> swapchainImageViews;
    VkPipelineLayout pipelineLayout;

    void* VulkanLibrary = nullptr;
    xcb_connection_t* connection;
    xcb_window_t handle;
    bool canRender;

    // SDL_Window* window;
    void* window;                    // Eric: void for now until I get the windowing part up and runnint
    char* extensionStringNames[64];  // Eric: why am I char const *?
    uint32_t extensionCount = 0;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        bool is_complete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    QueueFamilyIndices familyIndicies;

    void init_window();
    void init_vulkan();
    void init_input();

    std::vector<const char*> get_required_extensions();
    SwapChainSupportDetails query_swap_chain_support(VkPhysicalDevice device);
    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);
    bool check_validation_layer_support();
    bool check_device_extension_support(VkPhysicalDevice device);
    bool is_device_suitable(VkPhysicalDevice device);
    void get_device_queues();
    VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes);
    VkExtent2D choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities);

    void create_instance();
    void pick_physical_device();
    void create_logical_device();
    void create_surface();
    void create_swap_chain();
    void create_image_views();
    void create_graphics_pipeline();

    void setup_debug_callback();

    bool rendering_loop();
    bool on_window_size_changed() { return true; }
    bool draw();
    bool ready_to_draw() { return canRender; }

    bool load_vulkan_library();
    bool load_exported_entry_points();
    bool load_global_entry_points();
    bool load_instance_level_entry_points();
    bool load_device_entry_level_points();

    static std::vector<char> read_file(const std::string& filename);
    VkShaderModule create_shader_module(const std::vector<char>& code);

    void cleanup();
};
}}      // namespace rake::vlkn
#endif  // SKELETONAPP_H