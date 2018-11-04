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
/**
 * @brief Static list of application validation layers for Vulkan debugging
 *
 */
const std::vector<const char*> validationLayers = {"VK_LAYER_LUNARG_standard_validation"};

// Default instance extensions
const std::vector<const char*> instanceExtensions = {"VK_KHR_xcb_surface", "VK_KHR_surface"};

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

    void* VulkanLibrary = nullptr;
    // Vk Interface
    VkInstance instance;

    VkDebugUtilsMessengerEXT callback;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device;
    VkQueue graphicsQueue;
    VkSurfaceKHR surface;
    VkQueue presentQueue;

    xcb_connection_t* connection;
    xcb_window_t handle;

    // SDL_Window* window;
    void* window;                    // Eric: void for now until I get the windowing part up and runnint
    char* extensionStringNames[64];  // Eric: why am I char const *?
    uint32_t extensionCount = 0;

    struct QueueFamilyIndices {
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;

        bool is_complete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
    };

    QueueFamilyIndices familyIndicies;

    bool canRender;
    void init_window();
    void init_input();
    bool rendering_loop();

    bool on_window_size_changed() { return true; }
    bool draw() { return true; }
    bool ready_to_draw() { return canRender; }
    std::vector<const char*> get_required_extensions();
    bool check_validation_layer_support();
    void create_instance();
    void pick_physical_device();

    bool is_device_suitable(VkPhysicalDevice device);

    QueueFamilyIndices find_queue_families(VkPhysicalDevice device);

    void create_logical_device();
    void get_device_queues();
    void create_surface();
    void init_vulkan();
    void setup_debug_callback();
    void cleanup();

    bool load_vulkan_library();
    bool load_exported_entry_points();
    bool load_global_entry_points();
    bool load_instance_level_entry_points();
    bool load_device_entry_level_points();
};
}}      // namespace rake::vlkn
#endif  // SKELETONAPP_H