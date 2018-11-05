#include <iostream>
#include <limits>
#include <sstream>
#include <string>

// #include <thread>
// #include <chrono>

#include "vktutorialapp.h"
#include "config.h"

#include "vulkan/VulkanFunctions.h"

namespace rake { namespace vlkn {

/**
 * @brief Create a vkDebugUtilsMessengerEXT object
 *
 * @param instance
 * @param pCreateInfo
 * @param pAllocator
 * @param pCallback
 * @return VkResult
 */
VkResult create_debug_util_messenger_ext(VkInstance instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks* pAllocator,
                                         VkDebugUtilsMessengerEXT* pCallback)
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pCallback);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

/**
 * @brief Make sure to clean up any vkDebugUtilsMessengerEXT objects we created
 *
 * @param instance
 * @param callback
 * @param pAllocator
 */
void destroy_debug_utils_messenger_ext(VkInstance instance,
                                       VkDebugUtilsMessengerEXT callback,
                                       const VkAllocationCallbacks* pAllocator)
{
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, callback, pAllocator);
    }
}

/**
 * @brief Construct a new vk Tutorial App::vk Tutorial App object
 * 
 */
vkTutorialApp::vkTutorialApp()
    : connection()
    , handle()
    , canRender(false)
{
    version({Major, Minor, Patch, Compile});
    name(project_name);
    app_description.push_back(std::string("vkTutorialApp \n"));
    app_description.push_back(std::string("** nothing else follows ** \n"));
}

/**
 * @brief // All of the application starts here if any parameters 
 * are used they are handled with member vars.
 * 
 * @return int 
 */
int vkTutorialApp::main()
{
    // SDL_Event event;
    // Start Main Application Here.
    init_window();
    init_vulkan();

    /*
    while (event.type != SDL_QUIT) {
        SDL_PollEvent(&event);
    }
*/
    if (!rendering_loop()) {
        cleanup();
        return -1;
    }
    cleanup();
    return 0;
}

/**
 * @brief This is the main that handles parameters
 * 
 * @param params 
 * @return int 
 */
int vkTutorialApp::main(std::vector<std::string>& params)
{
    // std::vector<std::string> actions;
    std::vector<std::string> dump;

    // iterate through params to remove the -- from the text
    for (std::vector<std::string>::const_iterator i = params.begin(); i != params.end(); ++i) {
        if (*i == "--help" || *i == "-h") {
            actions.push_back("help");
        } else if (*i == "--verbose" || *i == "-v") {
            actions.push_back("verbose");
        } else if (*i == "--version" || *i == "-V") {
            actions.push_back("version");
        } else {  // catch all to make sure there are no invalid parameters
            dump.push_back(*i);
        }
    }

    for (auto& c : actions) {  // handle all the prameters
        if (c == "help") {
            help();
            return 0;  // exit the app
        } else if (c == "verbose") {
            for (auto& d : actions) {
                std::cout << "--" << d << " ";
            }
            std::cout << std::endl;
            return 0;
        } else if (c == "version") {
            std::cout << app_name << " version " << version() << "\n";
            std::cout << "Compiler: " << compiler_name << " " << compiler_version << "\n";
            std::cout << "Operating System: " << operating_system << "\n";
            std::cout << "Architecture: " << cpu_family << std::endl;
            return 0;
        } else {
            dump.push_back(c);
        }
    }

    return main();
}

/**
 * @brief This main converts c style parameters to c++ strings
 * then passes it to main that handles the actual parametrs.
 * 
 * @param argv 
 * @param argc 
 * @return int 
 */
int vkTutorialApp::main(int argv, char* argc[])
{
    // Start here if there are params
    std::vector<std::string> params;

    for (int i = 1; i != argv; ++i) {
        params.push_back(argc[i]);
    }

    return main(params);
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::help(void)
{
    std::cout << "Usage: " << app_name << " [options] files...\n\n";
    std::cout << "Options: \n";
    std::cout << " -h, --help \t\t Print this help message and exit the program.\n";
    std::cout << " -v, --verbose \t\t Print out all the valid command line "
                 "parameters\n";
    std::cout << " \t\t\t passed to the program.\n";
    std::cout << " -V, --version \t\t Print the version and exit.\n";
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::init_window()
{
    int screen_index = 0;
    connection = xcb_connect(nullptr, &screen_index);

    if (!connection) {
        std::runtime_error("Error in XCB connection creation!");
        return;
    }

    const xcb_setup_t* setup = xcb_get_setup(connection);
    xcb_screen_iterator_t screen_iterator = xcb_setup_roots_iterator(setup);

    while (screen_index-- > 0) {
        xcb_screen_next(&screen_iterator);
    }

    xcb_screen_t* screen = screen_iterator.data;
    handle = xcb_generate_id(connection);

    uint32_t value_list[] = {screen->white_pixel,
                             XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY};

    xcb_create_window(connection, XCB_COPY_FROM_PARENT, handle, screen->root, 29, 29, this->width, this->height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK,
                      value_list);

    xcb_flush(connection);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, handle, XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                        strlen(app_name.c_str()), app_name.c_str());
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::init_input()
{
    // SDL_Init(SDL_INIT_EVENTS);
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::rendering_loop()
{
    xcb_intern_atom_cookie_t protocols_cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* protocols_reply = xcb_intern_atom_reply(connection, protocols_cookie, 0);
    xcb_intern_atom_cookie_t delete_cookie = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* delete_reply = xcb_intern_atom_reply(connection, delete_cookie, 0);
    xcb_change_property(connection, XCB_PROP_MODE_REPLACE, handle, (*protocols_reply).atom, 4, 32, 1,
                        &(*delete_reply).atom);
    free(protocols_reply);

    // Display window
    xcb_map_window(connection, handle);
    xcb_flush(connection);

    // Main message loop
    xcb_generic_event_t* xevent;
    bool loop = true;
    bool resize = false;
    bool result = true;

    while (loop) {
        xevent = xcb_poll_for_event(connection);

        if (xevent) {
            // Process event
            switch (xevent->response_type & 0x7f) {
                // resize
                case XCB_CONFIGURE_NOTIFY: {
                    xcb_configure_notify_event_t* configure_event = (xcb_configure_notify_event_t*)xevent;
                    static uint16_t width = configure_event->width;
                    static uint16_t height = configure_event->height;

                    if (((configure_event->width > 0) && (width != configure_event->width)) ||
                        ((configure_event->height > 0) && height != configure_event->height)) {
                        resize = true;
                        width = configure_event->width;
                        height = configure_event->height;
                    }

                } break;
                //close
                case XCB_CLIENT_MESSAGE:
                    if ((*(xcb_client_message_event_t*)xevent).data.data32[0] == (*delete_reply).atom) {
                        loop = false;
                        free(delete_reply);
                    }
                    break;
                case XCB_KEY_PRESS:
                    loop = false;
                    break;
            }
            free(xevent);
        } else {
            // Draw
            if (resize) {
                resize = false;
                if (!on_window_size_changed()) {
                    result = false;
                    break;
                }
            }
            if (ready_to_draw()) {
                if (!draw()) {
                    result = false;
                    break;
                }
            } else {
                // std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }
    return result;
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::draw()
{ /*
    uint32_t image_index;
    VkResult result = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, Eric: semephore VK_NULL_HANDLE,
                                            Eric: fence VK_NULL_HANDLE, &image_index);
    switch (result) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            return on_window_size_changed();
        default:
            std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
            return false;
    } */
    return true;
}

/**
* @brief Get the VkExtensionProperties object and fill extensionNames and
* extensionCount
*
*/
std::vector<const char*> vkTutorialApp::get_required_extensions()
{
    uint32_t instanceExtensionCount = 0;
    std::vector<VkExtensionProperties> vkInstanceExtensions;
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, nullptr);
    vkInstanceExtensions.resize(instanceExtensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &instanceExtensionCount, vkInstanceExtensions.data());

    std::cout << "\nNumber of availiable instance extensions\t" << vkInstanceExtensions.size() << "\n";
    std::cout << "Available Extension List: \n";
    for (auto& ext : vkInstanceExtensions) {
        std::cout << "\t" << ext.extensionName << "\n";
    }
    std::cout << std::endl;

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
    std::cout << std::endl;

    return extensions;
}

/**
	 * @brief Check for Validation Layer support
	 *
	 * @return true
	 * @return false
	 */
bool vkTutorialApp::check_validation_layer_support()
{
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    std::vector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    for (const char* layerName : validationLayers) {
        bool layerFound = false;

        for (auto& layerProperties : availableLayers) {
            if (strcmp(layerName, layerProperties.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            return false;
        }
    }
    return true;
}

/**
	 * @brief Create a Instance object
	 *
	 */
void vkTutorialApp::create_instance()
{
    if (enableValidationLayers && !check_validation_layer_support()) {
        throw std::runtime_error("\tValidation layers requested, but not available!");
    }

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle.";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.pEngineName = "No Engine.";
    appInfo.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    appInfo.apiVersion = VK_API_VERSION_1_1;
    appInfo.pNext = nullptr;

    VkInstanceCreateInfo createInfo = {};

    auto extensions = get_required_extensions();

    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = extensions.size();
    createInfo.ppEnabledExtensionNames = extensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::runtime_error("vkCreateInstance failed\n");
    }
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::pick_physical_device()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support.");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    for (const auto& device : devices) {
        if (is_device_suitable(device)) {
            physicalDevice = device;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a sutible GPU!");
    }
}

/**
 * @brief 
 * 
 * @param device 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::is_device_suitable(VkPhysicalDevice device)
{
    QueueFamilyIndices indicies = find_queue_families(device);
    bool extensionSupported = check_device_extension_support(device);
    bool swapChainAdequate = false;

    if (extensionSupported) {
        SwapChainSupportDetails swapChainSupport = query_swap_chain_support(device);
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indicies.is_complete() && extensionSupported && swapChainAdequate;
}

/**
 * @brief 
 * 
 * @param device 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::check_device_extension_support(VkPhysicalDevice device)
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
 * 
 * @param device 
 * @return rake::vlkn::vkTutorialApp::SwapChainSupportDetails 
 */
rake::vlkn::vkTutorialApp::SwapChainSupportDetails vkTutorialApp::query_swap_chain_support(VkPhysicalDevice device)
{
    SwapChainSupportDetails details;
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
 * @return vkTutorialApp::QueueFamilyIndices 
 */
vkTutorialApp::QueueFamilyIndices vkTutorialApp::find_queue_families(VkPhysicalDevice device)
{
    QueueFamilyIndices indices;

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
 * @param availableFormats 
 * @return VkSurfaceFormatKHR 
 */
VkSurfaceFormatKHR vkTutorialApp::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    if (availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED) {
        return {VK_FORMAT_B8G8R8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};
    }

    for (const auto& availableFomat : availableFormats) {
        if (availableFomat.format == VK_FORMAT_B8G8R8_UNORM &&
            availableFomat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return availableFomat;
        }
    }

    return availableFormats[0];
}

/**
 * @brief 
 * 
 * @param availablePresentMode 
 * @return VkPresentModeKHR 
 */
VkPresentModeKHR vkTutorialApp::choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto& availablePresentMode : availablePresentModes) {
        if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
            return availablePresentMode;
        } else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
            bestMode = availablePresentMode;
        }
    }

    return bestMode;
}

VkExtent2D vkTutorialApp::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
        return capabilities.currentExtent;
    } else {
        VkExtent2D actualExtent = {this->width, this->height};

        actualExtent.width = std::max(capabilities.minImageExtent.width,
                                      std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height = std::max(capabilities.minImageExtent.height,
                                       std::min(capabilities.maxImageExtent.height, actualExtent.height));
    }
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::create_logical_device()
{
    familyIndicies = find_queue_families(physicalDevice);

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {familyIndicies.graphicsFamily.value(),
                                              familyIndicies.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount = queueCreateInfos.size();
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = validationLayers.size();
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::get_device_queues()
{
    vkGetDeviceQueue(device, familyIndicies.presentFamily.value(), 0, &presentQueue);
    vkGetDeviceQueue(device, familyIndicies.graphicsFamily.value(), 0, &graphicsQueue);
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::create_surface()
{
    VkXcbSurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                                                     connection, handle};

    if (vkCreateXcbSurfaceKHR(instance, &surface_create_info, nullptr, &surface) == VK_SUCCESS) {
        return;
    }
}

/**
* @brief init Vulkan
*
*/
void vkTutorialApp::init_vulkan()
{
    load_vulkan_library();
    load_exported_entry_points();
    load_global_entry_points();
    create_instance();
    load_instance_level_entry_points();
    setup_debug_callback();
    create_surface();
    pick_physical_device();
    create_logical_device();
    load_device_entry_level_points();
    get_device_queues();
    create_swap_chain();
    create_image_views();
    create_graphics_pipeline();
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::create_graphics_pipeline()
{
    auto vertShaderCode = read_file("triangle_vert.spv");
    auto fragShaderCode = read_file("triangle_frag.spv");

    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

    vertShaderModule = create_shader_module(vertShaderCode);
    fragShaderModule = create_shader_module(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;  // optional
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;  // optional

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapchainExtent.width;
    viewport.height = (float)swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f;  // optional
    rasterizer.depthBiasClamp = 0.0f;           // optional
    rasterizer.depthBiasSlopeFactor = 0.0f;     // optional

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f;           // optional
    multisampling.pSampleMask = nullptr;             // optional
    multisampling.alphaToCoverageEnable = VK_FALSE;  // optional
    multisampling.alphaToOneEnable = VK_FALSE;       // optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // optional
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // optional
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;              // optional
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // optional
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // optional
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;              // optional

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;  // optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;  // optional
    colorBlending.blendConstants[1] = 0.0f;  // optional
    colorBlending.blendConstants[2] = 0.0f;  // optional
    colorBlending.blendConstants[3] = 0.0f;  // optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;          // optional
    pipelineLayoutInfo.pSetLayouts = nullptr;       // optional
    pipelineLayoutInfo.pushConstantRangeCount = 0;  // optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

/**
 * @brief 
 * 
 * @param code 
 * @return VkShaderModule 
 */
VkShaderModule vkTutorialApp::create_shader_module(const std::vector<char>& code)
{
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create shader module!");
    }
    return shaderModule;
}

/**
 * @brief 
 * 
 */
void vkTutorialApp::create_image_views()
{
    swapchainImageViews.resize(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) {
        VkImageViewCreateInfo createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = swapchainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = swapchainImageFormat;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        if (vkCreateImageView(device, &createInfo, nullptr, &swapchainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create image views!");
        }
    }
}

void vkTutorialApp::create_swap_chain()
{
    SwapChainSupportDetails swapChainSupport = query_swap_chain_support(physicalDevice);
    VkSurfaceFormatKHR surfaceFomat = choose_swap_surface_format(swapChainSupport.formats);
    VkPresentModeKHR presentMode = choose_swap_present_mode(swapChainSupport.presentModes);
    VkExtent2D extent = choose_swap_extent(swapChainSupport.capabilities);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFomat.format;
    createInfo.imageColorSpace = surfaceFomat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indicies = find_queue_families(physicalDevice);
    uint32_t queueFamilyIndicies[] = {indicies.graphicsFamily.value(), indicies.presentFamily.value()};

    if (indicies.graphicsFamily != indicies.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndicies;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;      // optional?
        createInfo.pQueueFamilyIndices = nullptr;  // optional?
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create swap chain!");
    }

    canRender = true;
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

    swapchainImageFormat = surfaceFomat.format;
    swapchainExtent = extent;
}

/**
* @brief setupDebugCallback
*
*/
void vkTutorialApp::setup_debug_callback()
{
    if (!enableValidationLayers) {
        return;
    }

    VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debug_callback;
    createInfo.pUserData = nullptr;

    if (create_debug_util_messenger_ext(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug callback");
    }
}

/**
* @brief vkTutorial cleanup.
*
*/
void vkTutorialApp::cleanup()
{
    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
    for (auto imageView : swapchainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapchain, nullptr);
    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        destroy_debug_utils_messenger_ext(instance, callback, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
    if (connection != nullptr) {
        xcb_destroy_window(connection, handle);
        xcb_disconnect(connection);
    }
    dlclose(VulkanLibrary);
    // SDL_Quit();
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::load_vulkan_library()
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
    VulkanLibrary = dlopen("libvulkan.so.1", RTLD_NOW);
#endif

    if (VulkanLibrary == nullptr) {
        std::cout << "Could not load vulkan library!" << std::endl;
        return false;
    } else {
        return true;
    }
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::load_exported_entry_points()
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
#define LoadProcAddress dlsym
#endif

#define VK_EXPORTED_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)LoadProcAddress(VulkanLibrary, #fun))) {                    \
        std::cout << "Could not load exported function: " << #fun << "!" << std::endl; \
        return false;                                                                  \
    }

#include "vulkan/VulkanFunctions.inl"
    return true;
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::load_global_entry_points()
{
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {                        \
        std::cout << "Could not load global level function: " << #fun << "!" << std::endl; \
        return false;                                                                      \
    }

#include "vulkan/VulkanFunctions.inl"

    return true;
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::load_instance_level_entry_points()
{
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                                      \
    if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) {                         \
        std::cout << "Could not load instance level function: " << #fun << "!" << std::endl; \
        return false;                                                                        \
    }

#include "vulkan/VulkanFunctions.inl"
    return true;
}

/**
 * @brief 
 * 
 * @return true 
 * @return false 
 */
bool vkTutorialApp::load_device_entry_level_points()
{
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                                        \
    if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(device, #fun))) {                             \
        std::cout << "Could not load instance level function: " << #fun << "!" << std::endl; \
        return false;                                                                        \
    }

#include "vulkan/VulkanFunctions.inl"
    return true;
}

/**
 * @brief 
 * 
 * @param filename 
 * @return std::vector<char> 
 */
std::vector<char> vkTutorialApp::read_file(const std::string& filename)
{
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file!");
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

}}  // namespace rake::vlkn