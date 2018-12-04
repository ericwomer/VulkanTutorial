

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

#include <thread>
#include <chrono>
#include <unordered_map>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "VulkanFunctions.h"
#include "VulkanCommon.h"
#include "VulkanUtilities.h"

namespace Rake { namespace Graphics {

template <typename T>
std::unique_ptr<T> generate_unique_ptr()
{
  auto temp = std::make_unique<T>();
  return temp;
}

/**
* @brief Create a vkDebugUtilsMessengerEXT object
*
* @param instance
* @param pCreateInfo
* @param pAllocator
* @param pCallback
* @return VkResult
*/
VkResult create_debug_util_messenger_ext(VkInstance                                instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                         const VkAllocationCallbacks*              pAllocator,
                                         VkDebugUtilsMessengerEXT*                 pCallback)
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

void destroy_debug_utils_messenger_ext(VkInstance                   instance,
                                       VkDebugUtilsMessengerEXT     callback,
                                       const VkAllocationCallbacks* pAllocator)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  if (func != nullptr) {
    func(instance, callback, pAllocator);
  }
}

Core::Core()
{
  helper     = generate_unique_ptr<Helper>();
  filesystem = generate_unique_ptr<Filesystem>();

  chalet.width       = 800;
  chalet.height      = 600;
  chalet.modelPath   = "data/models/chalet.obj";
  chalet.texturePath = "data/textures/chalet.jpg";
}

Core::~Core()
{
}

/**
 * @brief
 */
void Core::cleanup()
{
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }

  cleanup_swapchain();

  vkDestroySampler(device, textureSampler, nullptr);
  vkDestroyImageView(device, textureImageView, nullptr);
  vkDestroyImage(device, textureImage, nullptr);
  vkFreeMemory(device, textureImageMemory, nullptr);

  vkDestroyDescriptorPool(device, descriptorPool, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    vkDestroyBuffer(device, uniformBuffers[i], nullptr);
    vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
  }

  vkDestroyBuffer(device, indexBuffer, nullptr);
  vkFreeMemory(device, indexBufferMemory, nullptr);
  vkDestroyBuffer(device, vertexBuffer, nullptr);
  vkFreeMemory(device, vertexBufferMemory, nullptr);

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphore[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);
  vkDestroyDevice(device, nullptr);

  if (enableValidationLayers) {
    destroy_debug_utils_messenger_ext(instance, callback, nullptr);
  }

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  dlclose(VulkanLibrary);
  // SDL_Quit();
}

void Core::cleanup_swapchain()
{
  switch (auto result = vkGetFenceStatus(device, inFlightFences[currentFrame]); result) {
    case VK_SUCCESS:
      break;
    case VK_NOT_READY:
      vkDeviceWaitIdle(device);
      break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    case VK_ERROR_DEVICE_LOST:
      throw std::runtime_error("Fence Status Error!");
      return;
  }

  vkDestroyImageView(device, colorImageView, nullptr);
  vkDestroyImage(device, colorImage, nullptr);
  vkFreeMemory(device, colorImageMemory, nullptr);

  for (auto framebuffer : swapchainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  vkDestroyImageView(device, depthImageView, nullptr);
  vkDestroyImage(device, depthImage, nullptr);
  vkFreeMemory(device, depthImageMemory, nullptr);

  vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
  commandBuffers.clear();

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  for (auto imageView : swapchainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  if (swapchain != VK_NULL_HANDLE) {
    vkDestroySwapchainKHR(device, swapchain, nullptr);
  }
}

/**
 * @brief
 * @return
 */
bool Core::on_window_size_changed()
{
  return recreate_swap_chain();  // ChildOnwindowSizeChanged();
}

/**
* @brief Create a Instance object
*
*/
void Core::create_instance()
{
  if (enableValidationLayers && !helper->check_validation_layer_support()) {
    throw std::runtime_error("\tValidation layers requested, but not available!");
  }

  VkApplicationInfo appInfo  = {};
  appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName   = "Hello Triangle.";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
  appInfo.pEngineName        = "No Engine.";
  appInfo.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
  appInfo.apiVersion         = VK_API_VERSION_1_1;
  appInfo.pNext              = nullptr;

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType                = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo     = &appInfo;

  auto extensions                    = helper->get_required_extensions();
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(extensions.size());
  createInfo.ppEnabledExtensionNames = extensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount   = static_cast<uint32_t>(validationLayers.size());
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
void Core::pick_physical_device()
{
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw std::runtime_error("failed to find GPUs with Vulkan support.");
  }

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto& device : devices) {
    if (helper->is_device_suitable(device, surface)) {
      physicalDevice = device;
      msaaSamples    = helper->get_max_usable_sample_count(physicalDevice);
    }
  }

  if (physicalDevice == VK_NULL_HANDLE) {
    throw std::runtime_error("failed to find a sutible GPU!");
  }
}

/**
* @brief
*
* @param availableFormats
* @return VkSurfaceFormatKHR
*/
VkSurfaceFormatKHR Core::choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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
VkPresentModeKHR Core::choose_swap_present_mode(const std::vector<VkPresentModeKHR> availablePresentModes)
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

/**
 * @brief
 * @param capabilities
 * @return
 */
VkExtent2D Core::choose_swap_extent(const VkSurfaceCapabilitiesKHR& capabilities)
{
  if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = {static_cast<uint32_t>(this->width), static_cast<uint32_t>(this->height)};

    actualExtent.width =
        std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
    actualExtent.height =
        std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
    return actualExtent;
  }
}

/**
 * @brief
 *
 */
void Core::create_logical_device()
{
  familyIndicies = helper->find_queue_families(physicalDevice, surface);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t>                   uniqueQueueFamilies = {familyIndicies.graphicsFamily.value(),
                                            familyIndicies.presentFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex        = queueFamily;
    queueCreateInfo.queueCount              = 1;
    queueCreateInfo.pQueuePriorities        = &queuePriority;
    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy        = VK_TRUE;
  deviceFeatures.sampleRateShading        = VK_TRUE;

  VkDeviceCreateInfo createInfo      = {};
  createInfo.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount    = queueCreateInfos.size();
  createInfo.pQueueCreateInfos       = queueCreateInfos.data();
  createInfo.pEnabledFeatures        = &deviceFeatures;
  createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();

  if (enableValidationLayers) {
    createInfo.enabledLayerCount   = validationLayers.size();
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
void Core::create_surface(xcb_connection_t* connection, xcb_window_t handle)
{
  VkXcbSurfaceCreateInfoKHR surface_create_info = {VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
                                                   nullptr,
                                                   0,
                                                   connection,
                                                   handle};

  if (auto result = vkCreateXcbSurfaceKHR(instance, &surface_create_info, nullptr, &surface); result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create surface!");
  }
}

/**
 * @brief init Vulkan
 *
 */
void Core::init_vulkan(xcb_connection_t* connection, xcb_window_t handle)
{
  load_vulkan_library();
  load_exported_entry_points();
  load_global_entry_points();
  create_instance();
  load_instance_level_entry_points();
  setup_debug_callback();
  create_surface(connection, handle);
  pick_physical_device();
  create_logical_device();
  load_device_entry_level_points();
  helper->get_device_queues(device, familyIndicies, presentQueue, graphicsQueue);
  create_swap_chain();
  create_image_views();
  create_render_pass();
  create_descriptor_set_layout();
  create_graphics_pipeline();
  create_command_pool();
  create_color_resources();
  create_depth_resources();
  create_frame_buffer();
  create_texture_image();
  create_texture_image_view();
  create_texture_sampler();
  filesystem->load_model(chalet);
  create_vertex_buffer();
  create_index_buffer();
  create_uniform_buffers();
  create_descriptor_pool();
  create_descriptor_sets();
  create_command_buffers();
  create_sync_objects();
}

/**
 * @brief
 */
void Core::create_color_resources()
{
  VkFormat colorFormat = swapchainImageFormat;

  create_image(swapchainExtent.width,
               swapchainExtent.height,
               1,
               msaaSamples,
               colorFormat,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               colorImage,
               colorImageMemory);
  colorImageView = create_image_view(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
  transition_image_layout(colorImage,
                          colorFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
                          1);
}

/**
 * @brief
 */
void Core::create_depth_resources()
{
  VkFormat depthFormat = helper->find_depth_format(physicalDevice);

  create_image(swapchainExtent.width,
               swapchainExtent.height,
               1,
               msaaSamples,
               depthFormat,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               depthImage,
               depthImageMemory);
  depthImageView = create_image_view(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

  transition_image_layout(depthImage,
                          depthFormat,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                          1);
}

/**
 * @brief
 */
void Core::create_texture_sampler()
{
  VkSamplerCreateInfo samplerInfo     = {};
  samplerInfo.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
  samplerInfo.magFilter               = VK_FILTER_LINEAR;
  samplerInfo.minFilter               = VK_FILTER_LINEAR;
  samplerInfo.addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT;
  samplerInfo.anisotropyEnable        = VK_TRUE;
  samplerInfo.maxAnisotropy           = 16;
  samplerInfo.borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
  samplerInfo.unnormalizedCoordinates = VK_FALSE;
  samplerInfo.compareEnable           = VK_FALSE;
  samplerInfo.compareOp               = VK_COMPARE_OP_ALWAYS;
  samplerInfo.mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR;
  samplerInfo.mipLodBias              = 0.0f;
  samplerInfo.minLod                  = 0.0f;
  samplerInfo.maxLod                  = static_cast<uint32_t>(mipLevels);

  if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create texture sampler!");
  }
}

/**
 * @brief
 */
void Core::create_texture_image_view()
{
  textureImageView = create_image_view(textureImage, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
}

/**
 * @brief
 * @param image
 * @param format
 * @return
 */
VkImageView Core::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels)
{
  VkImageViewCreateInfo viewInfo           = {};
  viewInfo.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
  viewInfo.image                           = image;
  viewInfo.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
  viewInfo.format                          = format;
  viewInfo.subresourceRange.aspectMask     = aspectFlags;
  viewInfo.subresourceRange.baseMipLevel   = 0;
  viewInfo.subresourceRange.levelCount     = mipLevels;
  viewInfo.subresourceRange.baseArrayLayer = 0;
  viewInfo.subresourceRange.layerCount     = 1;

  VkImageView imageView;
  if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create texture image view!");
  }
  return imageView;
}

/**
 * @brief
 */
void Core::create_texture_image()
{
  int          texWidth, texHeight, texChannels;
  stbi_uc*     pixels    = stbi_load(chalet.texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
  VkDeviceSize imageSize = texWidth * texHeight * 4;

  mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

  if (!pixels) {
    throw std::runtime_error("Failed to load texture image!");
  }

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  create_buffer(imageSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
  memcpy(data, pixels, static_cast<size_t>(imageSize));
  vkUnmapMemory(device, stagingBufferMemory);

  stbi_image_free(pixels);

  create_image(texWidth,
               texHeight,
               mipLevels,
               VK_SAMPLE_COUNT_1_BIT,
               VK_FORMAT_R8G8B8A8_UNORM,
               VK_IMAGE_TILING_OPTIMAL,
               VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
               textureImage,
               textureImageMemory);

  transition_image_layout(textureImage,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          mipLevels);

  copy_buffer_to_image(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
  /*
  transition_image_layout(textureImage,
                          VK_FORMAT_R8G8B8A8_UNORM,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                          mipLevels);
*/

  generate_mipmaps(textureImage, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/**
 * @brief
 * @param image
 * @param texWidth
 * @param texHeight
 * @param mipLevels
 */
void Core::generate_mipmaps(VkImage  image,
                            VkFormat imageFormat,
                            int32_t  texWidth,
                            int32_t  texHeight,
                            uint32_t mipLevels)
{
  VkFormatProperties formatProperties;
  vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

  if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
    throw std::runtime_error("Texture image format does not support linear blitting!");
  }

  VkCommandBuffer commandBuffer = begin_single_time_commands();

  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.image                           = image;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;
  barrier.subresourceRange.levelCount     = 1;

  int32_t mipWidth  = texWidth;
  int32_t mipHeight = texHeight;

  for (uint32_t i = 1; i < mipLevels; i++) {
    barrier.subresourceRange.baseMipLevel = i - 1;
    barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    VkImageBlit blit                   = {};
    blit.srcOffsets[0]                 = {0, 0, 0};
    blit.srcOffsets[1]                 = {mipWidth, mipHeight, 1};
    blit.srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.mipLevel       = i - 1;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount     = 1;
    blit.dstOffsets[0]                 = {0, 0, 0};
    blit.dstOffsets[1]                 = {mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1};
    blit.dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.mipLevel       = i;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount     = 1;

    vkCmdBlitImage(commandBuffer,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1,
                   &blit,
                   VK_FILTER_LINEAR);

    barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0,
                         nullptr,
                         0,
                         nullptr,
                         1,
                         &barrier);

    if (mipWidth > 1) {
      mipWidth /= 2;
    }
    if (mipHeight > 1) {
      mipHeight /= 2;
    }
  }

  barrier.subresourceRange.baseMipLevel = mipLevels - 1;
  barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
  barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

  vkCmdPipelineBarrier(commandBuffer,
                       VK_PIPELINE_STAGE_TRANSFER_BIT,
                       VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                       0,
                       0,
                       nullptr,
                       0,
                       nullptr,
                       1,
                       &barrier);

  end_single_time_commands(commandBuffer);
}

/**
 * @brief
 * @param width
 * @param height
 * @param format
 * @param tiling
 * @param usage
 * @param properties
 * @param image
 * @param imageMemory
 */
void Core::create_image(uint32_t              width,
                        uint32_t              height,
                        uint32_t              mipLevels,
                        VkSampleCountFlagBits numSamples,
                        VkFormat              format,
                        VkImageTiling         tiling,
                        VkImageUsageFlags     usage,
                        VkMemoryPropertyFlags properties,
                        VkImage&              image,
                        VkDeviceMemory&       imageMemory)
{
  VkImageCreateInfo imageInfo = {};
  imageInfo.sType             = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
  imageInfo.imageType         = VK_IMAGE_TYPE_2D;
  imageInfo.extent.width      = width;
  imageInfo.extent.height     = height;
  imageInfo.extent.depth      = 1;
  imageInfo.mipLevels         = mipLevels;
  imageInfo.arrayLayers       = 1;
  imageInfo.format            = format;
  imageInfo.tiling            = tiling;
  imageInfo.initialLayout     = VK_IMAGE_LAYOUT_UNDEFINED;
  imageInfo.usage             = usage;
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;
  imageInfo.samples           = numSamples;
  imageInfo.flags             = 0;  // Optional
  imageInfo.sharingMode       = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create image");
  }

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = helper->find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate image memory!");
  }

  vkBindImageMemory(device, image, imageMemory, 0);
}

/**
 * @brief
 */
void Core::create_descriptor_sets()
{
  std::vector<VkDescriptorSetLayout> layouts(swapchainImages.size(), descriptorSetLayout);

  VkDescriptorSetAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  allocInfo.descriptorPool              = descriptorPool;
  allocInfo.descriptorSetCount          = static_cast<uint32_t>(swapchainImages.size());
  allocInfo.pSetLayouts                 = layouts.data();

  descriptorSets.resize(swapchainImages.size());
  if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate descriptor sets");
  }

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer                 = uniformBuffers[i];
    bufferInfo.offset                 = 0;
    bufferInfo.range                  = sizeof(Object::UniformBufferObject);

    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout           = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView             = textureImageView;
    imageInfo.sampler               = textureSampler;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

    descriptorWrites[0].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet           = descriptorSets[i];
    descriptorWrites[0].dstBinding       = 0;
    descriptorWrites[0].dstArrayElement  = 0;
    descriptorWrites[0].descriptorType   = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrites[0].descriptorCount  = 1;
    descriptorWrites[0].pBufferInfo      = &bufferInfo;
    descriptorWrites[0].pImageInfo       = nullptr;  // Optional
    descriptorWrites[0].pTexelBufferView = nullptr;  // Optional

    descriptorWrites[1].sType            = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet           = descriptorSets[i];
    descriptorWrites[1].dstBinding       = 1;
    descriptorWrites[1].dstArrayElement  = 0;
    descriptorWrites[1].descriptorType   = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites[1].descriptorCount  = 1;
    descriptorWrites[1].pBufferInfo      = nullptr;
    descriptorWrites[1].pImageInfo       = &imageInfo;  // Optional
    descriptorWrites[1].pTexelBufferView = nullptr;     // Optional

    vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
  }
}

/**
 * @brief
 */
void Core::create_descriptor_pool()
{
  std::array<VkDescriptorPoolSize, 2> poolSizes = {};
  poolSizes[0].type                             = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  poolSizes[0].descriptorCount                  = static_cast<uint32_t>(swapchainImages.size());
  poolSizes[1].type                             = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  poolSizes[1].descriptorCount                  = static_cast<uint32_t>(swapchainImages.size());

  VkDescriptorPoolCreateInfo poolInfo = {};
  poolInfo.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  poolInfo.poolSizeCount              = static_cast<uint32_t>(poolSizes.size());
  poolInfo.pPoolSizes                 = poolSizes.data();
  poolInfo.maxSets                    = static_cast<uint32_t>(swapchainImages.size());

  if (auto result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool); result != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor pool!");
  }
}

/**
 * @brief
 */
void Core::create_uniform_buffers()
{
  VkDeviceSize bufferSize = sizeof(Object::UniformBufferObject);

  uniformBuffers.resize(swapchainImages.size());
  uniformBuffersMemory.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    create_buffer(bufferSize,
                  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  uniformBuffers[i],
                  uniformBuffersMemory[i]);
  }
}

/**
 * @brief
 * @return
 */
bool Core::recreate_swap_chain()
{
  if (device != VK_NULL_HANDLE) {
    vkDeviceWaitIdle(device);
  }

  cleanup_swapchain();
  create_swap_chain();

  if (canRender) {
    create_image_views();
    create_render_pass();
    create_graphics_pipeline();
    create_color_resources();
    create_depth_resources();
    create_frame_buffer();
    create_command_buffers();
    return true;
  }
  return false;
}

VkCommandBuffer Core::begin_single_time_commands()
{
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandPool                 = commandPool;
  allocInfo.commandBufferCount          = 1;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
  vkBeginCommandBuffer(commandBuffer, &beginInfo);

  return commandBuffer;
}

void Core::end_single_time_commands(VkCommandBuffer commandBuffer)
{
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo       = {};
  submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffer;

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
  vkQueueWaitIdle(graphicsQueue);

  vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

/**
 * @brief
 * @param srcBuffer
 * @param dstBuffer
 * @param size
 */
void Core::copy_buffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
  VkCommandBuffer commandBuffer = begin_single_time_commands();

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset    = 0;  // Optional
  copyRegion.dstOffset    = 0;  // Optional
  copyRegion.size         = size;

  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
  end_single_time_commands(commandBuffer);
}

/**
 * @brief
 * @param image
 * @param format
 * @param oldLayout
 * @param newLayout
 */
void Core::transition_image_layout(VkImage       image,
                                   VkFormat      format,
                                   VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   uint32_t      mipLevels)
{
  VkCommandBuffer      commandBuffer = begin_single_time_commands();
  VkPipelineStageFlags sourceStage;
  VkPipelineStageFlags destinationStage;

  VkImageMemoryBarrier barrier            = {};
  barrier.sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
  barrier.oldLayout                       = oldLayout;
  barrier.newLayout                       = newLayout;
  barrier.srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED;
  barrier.image                           = image;
  barrier.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  barrier.subresourceRange.baseMipLevel   = 0;
  barrier.subresourceRange.levelCount     = mipLevels;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount     = 1;

  if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    if (helper->has_stencil_component(format)) {
      barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }
  } else {
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  }

  if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL &&
             newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TRANSFER_BIT;
    destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    sourceStage      = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  } else {
    throw std::runtime_error("Unsupported layout transistion!");
  }

  vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

  end_single_time_commands(commandBuffer);
}  // namespace Application

/**
 * @brief
 * @param buffer
 * @param image
 * @param width
 * @param height
 */
void Core::copy_buffer_to_image(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height)
{
  VkCommandBuffer commandBuffer = begin_single_time_commands();

  VkBufferImageCopy region = {};
  region.bufferOffset      = 0;
  region.bufferRowLength   = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel       = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount     = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  end_single_time_commands(commandBuffer);
}

/**
 * @brief
 */
void Core::create_vertex_buffer()
{
  VkDeviceSize bufferSize = sizeof(chalet.verticies[0]) * chalet.verticies.size();

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  create_buffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, chalet.verticies.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  create_buffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                vertexBuffer,
                vertexBufferMemory);

  copy_buffer(stagingBuffer, vertexBuffer, bufferSize);
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

/**
 * @brief
 */
void Core::create_index_buffer()
{
  VkDeviceSize bufferSize = sizeof(chalet.indices[0]) * chalet.indices.size();

  VkBuffer       stagingBuffer;
  VkDeviceMemory stagingBufferMemory;

  create_buffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer,
                stagingBufferMemory);

  void* data;
  vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
  memcpy(data, chalet.indices.data(), (size_t)bufferSize);
  vkUnmapMemory(device, stagingBufferMemory);

  create_buffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                indexBuffer,
                indexBufferMemory);

  copy_buffer(stagingBuffer, indexBuffer, bufferSize);

  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void Core::update_uniform_buffer(uint32_t currentImage)
{
  static auto startTime = std::chrono::high_resolution_clock::now();

  auto  currentTime = std::chrono::high_resolution_clock::now();
  float time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

  struct Object::UniformBufferObject ubo = {};
  ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
  ubo.proj  = glm::perspective(glm::radians(45.0f), swapchainExtent.width / (float)swapchainExtent.height, 0.1f, 10.0f);
  ubo.proj[1][1] *= -1;

  void* data;
  vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
  memcpy(data, &ubo, sizeof(ubo));
  vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Core::draw()  // Eric: Draw is draw frame.
{
  vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());

  uint32_t imageIndex;
  VkResult result = vkAcquireNextImageKHR(device,
                                          swapchain,
                                          std::numeric_limits<uint64_t>::max(),
                                          imageAvailableSemaphore[currentFrame],
                                          VK_NULL_HANDLE,
                                          &imageIndex);

  switch (result) {
    case VK_SUCCESS:
    case VK_SUBOPTIMAL_KHR:
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
      return on_window_size_changed();
      break;
    default:
      std::cout << "Problem occurred during swap chain image acquisition!" << std::endl;
      return EXIT_FAILURE;
  }

  update_uniform_buffer(imageIndex);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType        = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore          waitSemaphores[] = {imageAvailableSemaphore[currentFrame]};
  VkPipelineStageFlags waitStages[]     = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores    = waitSemaphores;
  submitInfo.pWaitDstStageMask  = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];

  VkSemaphore signalSemaphores[]  = {renderFinishedSemaphore[currentFrame]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores    = signalSemaphores;

  vkResetFences(device, 1, &inFlightFences[currentFrame]);
  if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
    throw std::runtime_error("Failed to submit draw command buffer!");
  }

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType            = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores    = signalSemaphores;

  VkSwapchainKHR swapchains[] = {swapchain};
  presentInfo.swapchainCount  = 1;
  presentInfo.pSwapchains     = swapchains;
  presentInfo.pImageIndices   = &imageIndex;

  presentInfo.pResults = nullptr;  // Optional

  result = vkQueuePresentKHR(presentQueue, &presentInfo);

  switch (result) {
    case VK_SUCCESS:
      break;
    case VK_ERROR_OUT_OF_DATE_KHR:
    case VK_SUBOPTIMAL_KHR:
      return on_window_size_changed();
      break;
    default:
      throw std::runtime_error("Problem occurred during image presentation!\n");
      return false;
  }

  currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
  return true;
}

/**
 * @brief
 */
void Core::create_sync_objects()
{
  imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType                 = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType             = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags             = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i])) {
      throw std::runtime_error("Failed to create sync objects for a frame!");
    }
  }
}

/**
 * @brief Helper method to create memory buffers
 * @param size
 * @param usage
 * @param properties
 * @param buffer
 * @param bufferMemory
 */
void Core::create_buffer(VkDeviceSize          size,
                         VkBufferUsageFlags    usage,
                         VkMemoryPropertyFlags properties,
                         VkBuffer&             buffer,
                         VkDeviceMemory&       bufferMemory)
{
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType              = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size               = size;
  bufferInfo.usage              = usage;
  bufferInfo.sharingMode        = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType                = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize       = memRequirements.size;
  allocInfo.memoryTypeIndex      = helper->find_memory_type(physicalDevice, memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate buffer memory!");
  }

  vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void Core::create_command_buffers()
{
  commandBuffers.resize(swapchainFramebuffers.size());

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType                       = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool                 = commandPool;
  allocInfo.level                       = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount          = (uint32_t)commandBuffers.size();

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw std::runtime_error("Failed to allocate command buffers!");
  }

  for (size_t i = 0; i < commandBuffers.size(); i++) {
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType                    = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags                    = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    beginInfo.pInheritanceInfo         = nullptr;  // OptionalvkCmdDraw
    if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
      throw std::runtime_error("Failed to begin recording command buffers!");
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType                 = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass            = renderPass;
    renderPassInfo.framebuffer           = swapchainFramebuffers[i];
    renderPassInfo.renderArea.offset     = {0, 0};
    renderPassInfo.renderArea.extent     = swapchainExtent;

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color                    = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil             = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues    = clearValues.data();

    vkCmdBeginRenderPass(commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

    VkBuffer     vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[]       = {0};
    vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
    vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

    vkCmdBindDescriptorSets(commandBuffers[i],
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0,
                            1,
                            &descriptorSets[i],
                            0,
                            nullptr);
    vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(chalet.indices.size()), 1, 0, 0, 0);
    vkCmdEndRenderPass(commandBuffers[i]);

    if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
      throw std::runtime_error("Failed recording command buffers!");
    }
  }
}

void Core::create_command_pool()
{
  QueueFamilyIndices queueFamilyIndices = helper->find_queue_families(physicalDevice, surface);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType                   = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex        = queueFamilyIndices.graphicsFamily.value();
  poolInfo.flags                   = 0;

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create command pool!");
  }
}

void Core::create_frame_buffer()
{
  swapchainFramebuffers.resize(swapchainImageViews.size());

  for (size_t i = 0; i < swapchainImageViews.size(); i++) {
    std::array<VkImageView, 3> attachments = {colorImageView, depthImageView, swapchainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo = {};
    framebufferInfo.sType                   = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebufferInfo.renderPass              = renderPass;
    framebufferInfo.attachmentCount         = static_cast<uint32_t>(attachments.size());
    framebufferInfo.pAttachments            = attachments.data();
    framebufferInfo.width                   = swapchainExtent.width;
    framebufferInfo.height                  = swapchainExtent.height;
    framebufferInfo.layers                  = 1;

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapchainFramebuffers[i])) {
      throw std::runtime_error("Failed to create framebuffer!");
    }
  }
}

void Core::create_render_pass()
{
  VkAttachmentDescription colorAttachment = {};
  colorAttachment.format                  = swapchainImageFormat;
  colorAttachment.samples                 = msaaSamples;
  colorAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  colorAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_STORE;
  colorAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachment.finalLayout             = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription depthAttachment = {};
  depthAttachment.format                  = helper->find_depth_format(physicalDevice);
  depthAttachment.samples                 = msaaSamples;
  depthAttachment.loadOp                  = VK_ATTACHMENT_LOAD_OP_CLEAR;
  depthAttachment.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  depthAttachment.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  depthAttachment.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  depthAttachment.finalLayout             = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentDescription colorAttachmentResolve = {};
  colorAttachmentResolve.format                  = swapchainImageFormat;
  colorAttachmentResolve.samples                 = VK_SAMPLE_COUNT_1_BIT;
  colorAttachmentResolve.loadOp                  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.storeOp                 = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentResolve.stencilLoadOp           = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  colorAttachmentResolve.stencilStoreOp          = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  colorAttachmentResolve.initialLayout           = VK_IMAGE_LAYOUT_UNDEFINED;
  colorAttachmentResolve.finalLayout             = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

  VkAttachmentReference colorAttachmentRef = {};
  colorAttachmentRef.attachment            = 0;
  colorAttachmentRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkAttachmentReference depthAttachmentRef = {};
  depthAttachmentRef.attachment            = 1;
  depthAttachmentRef.layout                = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorAttachmentResolveRef = {};
  colorAttachmentResolveRef.attachment            = 2;
  colorAttachmentResolveRef.layout                = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

  VkSubpassDescription subpass    = {};
  subpass.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpass.colorAttachmentCount    = 1;
  subpass.pColorAttachments       = &colorAttachmentRef;
  subpass.pDepthStencilAttachment = &depthAttachmentRef;
  subpass.pResolveAttachments     = &colorAttachmentResolveRef;

  VkSubpassDependency dependency = {};
  dependency.srcSubpass          = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass          = 0;
  dependency.srcStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask       = 0;
  dependency.dstStageMask        = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask       = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve};

  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType                  = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount        = static_cast<uint32_t>(attachments.size());
  renderPassInfo.pAttachments           = attachments.data();
  renderPassInfo.subpassCount           = 1;
  renderPassInfo.pSubpasses             = &subpass;
  renderPassInfo.dependencyCount        = 1;
  renderPassInfo.pDependencies          = &dependency;

  if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create render pass!");
  }
}

void Core::create_descriptor_set_layout()
{
  VkDescriptorSetLayoutBinding uboLayoutBinding = {};
  uboLayoutBinding.binding                      = 0;
  uboLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  uboLayoutBinding.descriptorCount              = 1;
  uboLayoutBinding.stageFlags                   = VK_SHADER_STAGE_VERTEX_BIT;
  uboLayoutBinding.pImmutableSamplers           = nullptr;  // Optional

  VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
  samplerLayoutBinding.binding                      = 1;
  samplerLayoutBinding.descriptorCount              = 1;
  samplerLayoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  samplerLayoutBinding.pImmutableSamplers           = nullptr;
  samplerLayoutBinding.stageFlags                   = VK_SHADER_STAGE_FRAGMENT_BIT;

  std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

  VkDescriptorSetLayoutCreateInfo layoutInfo = {};
  layoutInfo.sType                           = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  layoutInfo.bindingCount                    = static_cast<uint32_t>(bindings.size());
  layoutInfo.pBindings                       = bindings.data();

  if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create descriptor set layout!");
  }
}

/**
 * @brief
 *
 * @param code
 * @return VkShaderModule
 */
VkShaderModule Core::create_shader_module(const VkDevice& device, const std::vector<char>& code)
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

/**
* @brief
*
*/
void Core::create_graphics_pipeline()
{
  auto vertShaderCode = filesystem->read_file("triangle.vert.spv");
  auto fragShaderCode = filesystem->read_file("triangle.frag.spv");

  VkShaderModule vertShaderModule;
  VkShaderModule fragShaderModule;

  vertShaderModule = create_shader_module(device, vertShaderCode);
  fragShaderModule = create_shader_module(device, fragShaderCode);

  VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
  vertShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  vertShaderStageInfo.stage                           = VK_SHADER_STAGE_VERTEX_BIT;
  vertShaderStageInfo.module                          = vertShaderModule;
  vertShaderStageInfo.pName                           = "main";

  VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
  fragShaderStageInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  fragShaderStageInfo.stage                           = VK_SHADER_STAGE_FRAGMENT_BIT;
  fragShaderStageInfo.module                          = fragShaderModule;
  fragShaderStageInfo.pName                           = "main";

  VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType                                = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

  auto bindingDescription   = Object::Vertex::getBindingDescription();
  auto attributeDescription = Object::Vertex::getAttributesDescription();

  vertexInputInfo.vertexBindingDescriptionCount   = 1;
  vertexInputInfo.pVertexBindingDescriptions      = &bindingDescription;  // optional
  vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
  vertexInputInfo.pVertexAttributeDescriptions    = attributeDescription.data();  // optional

  VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
  inputAssembly.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  inputAssembly.topology                               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  inputAssembly.primitiveRestartEnable                 = VK_FALSE;

  VkViewport viewport = {};
  viewport.x          = 0.0f;
  viewport.y          = 0.0f;
  viewport.width      = (float)swapchainExtent.width;
  viewport.height     = (float)swapchainExtent.height;
  viewport.minDepth   = 0.0f;
  viewport.maxDepth   = 1.0f;

  VkRect2D scissor = {};
  scissor.offset   = {0, 0};
  scissor.extent   = swapchainExtent;

  VkPipelineViewportStateCreateInfo viewportState = {};
  viewportState.sType                             = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  viewportState.viewportCount                     = 1;
  viewportState.pViewports                        = &viewport;
  viewportState.scissorCount                      = 1;
  viewportState.pScissors                         = &scissor;

  VkPipelineRasterizationStateCreateInfo rasterizer = {};
  rasterizer.sType                                  = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  rasterizer.depthClampEnable                       = VK_FALSE;
  rasterizer.rasterizerDiscardEnable                = VK_FALSE;
  rasterizer.polygonMode                            = VK_POLYGON_MODE_FILL;
  rasterizer.lineWidth                              = 1.0f;
  rasterizer.cullMode                               = VK_CULL_MODE_BACK_BIT;
  rasterizer.frontFace                              = VK_FRONT_FACE_COUNTER_CLOCKWISE;
  rasterizer.depthBiasEnable                        = VK_FALSE;
  rasterizer.depthBiasConstantFactor                = 0.0f;  // optional
  rasterizer.depthBiasClamp                         = 0.0f;  // optional
  rasterizer.depthBiasSlopeFactor                   = 0.0f;  // optional

  VkPipelineMultisampleStateCreateInfo multisampling = {};
  multisampling.sType                                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  multisampling.sampleShadingEnable                  = VK_TRUE;
  multisampling.rasterizationSamples                 = msaaSamples;
  multisampling.minSampleShading                     = 0.2f;      // optional
  multisampling.pSampleMask                          = nullptr;   // optional
  multisampling.alphaToCoverageEnable                = VK_FALSE;  // optional
  multisampling.alphaToOneEnable                     = VK_FALSE;  // optional

  VkPipelineDepthStencilStateCreateInfo depthStencil = {};
  depthStencil.sType                                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  depthStencil.depthTestEnable                       = VK_TRUE;
  depthStencil.depthWriteEnable                      = VK_TRUE;
  depthStencil.depthCompareOp                        = VK_COMPARE_OP_LESS;
  depthStencil.depthBoundsTestEnable                 = VK_FALSE;
  depthStencil.minDepthBounds                        = 0.0f;  // Optional
  depthStencil.maxDepthBounds                        = 1.0f;  // Optional
  depthStencil.stencilTestEnable                     = VK_FALSE;
  depthStencil.front                                 = {};  // Optional
  depthStencil.back                                  = {};  // Optional

  VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
  colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  colorBlendAttachment.blendEnable         = VK_FALSE;
  colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;   // optional
  colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;  // optional
  colorBlendAttachment.colorBlendOp        = VK_BLEND_OP_ADD;       // optional
  colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;   // optional
  colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;  // optional
  colorBlendAttachment.alphaBlendOp        = VK_BLEND_OP_ADD;       // optional

  VkPipelineColorBlendStateCreateInfo colorBlending = {};
  colorBlending.sType                               = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  colorBlending.logicOpEnable                       = VK_FALSE;
  colorBlending.logicOp                             = VK_LOGIC_OP_COPY;  // optional
  colorBlending.attachmentCount                     = 1;
  colorBlending.pAttachments                        = &colorBlendAttachment;
  colorBlending.blendConstants[0]                   = 0.0f;  // optional
  colorBlending.blendConstants[1]                   = 0.0f;  // optional
  colorBlending.blendConstants[2]                   = 0.0f;  // optional
  colorBlending.blendConstants[3]                   = 0.0f;  // optional

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType                      = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount             = 1;                     // optional
  pipelineLayoutInfo.pSetLayouts                = &descriptorSetLayout;  // optional
  pipelineLayoutInfo.pushConstantRangeCount     = 0;                     // optional

  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create pipeline layout!");
  }

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType                        = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount                   = 2;
  pipelineInfo.pStages                      = shaderStages;
  pipelineInfo.pVertexInputState            = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState          = &inputAssembly;
  pipelineInfo.pViewportState               = &viewportState;
  pipelineInfo.pRasterizationState          = &rasterizer;
  pipelineInfo.pMultisampleState            = &multisampling;
  pipelineInfo.pDepthStencilState           = &depthStencil;  // optional
  pipelineInfo.pColorBlendState             = &colorBlending;
  pipelineInfo.pDynamicState                = nullptr;  // optional

  pipelineInfo.layout     = pipelineLayout;
  pipelineInfo.renderPass = renderPass;
  pipelineInfo.subpass    = 0;

  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;  // optional

  if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create graphics pipeline!");
  }

  vkDestroyShaderModule(device, fragShaderModule, nullptr);
  vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

/**
 * @brief
 *
 */
void Core::create_image_views()
{
  swapchainImageViews.resize(swapchainImages.size());

  for (size_t i = 0; i < swapchainImages.size(); i++) {
    swapchainImageViews[i] = create_image_view(swapchainImages[i], swapchainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
    canRender              = true;
  }
}

void Core::create_swap_chain()
{
  canRender = false;

  SwapChainSupportDetails swapChainSupport = helper->query_swap_chain_support(physicalDevice, surface);
  VkSurfaceFormatKHR      surfaceFomat     = choose_swap_surface_format(swapChainSupport.formats);
  VkPresentModeKHR        presentMode      = choose_swap_present_mode(swapChainSupport.presentModes);
  VkExtent2D              extent           = choose_swap_extent(swapChainSupport.capabilities);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    imageCount = swapChainSupport.capabilities.maxImageCount;
  }

  if ((extent.width == 0) || (extent.height == 0)) {
    return;
  }

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface                  = surface;
  createInfo.minImageCount            = imageCount;
  createInfo.imageFormat              = surfaceFomat.format;
  createInfo.imageColorSpace          = surfaceFomat.colorSpace;
  createInfo.imageExtent              = extent;
  createInfo.imageArrayLayers         = 1;
  createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indicies              = helper->find_queue_families(physicalDevice, surface);
  uint32_t           queueFamilyIndicies[] = {indicies.graphicsFamily.value(), indicies.presentFamily.value()};

  if (indicies.graphicsFamily != indicies.presentFamily) {
    createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    createInfo.pQueueFamilyIndices   = queueFamilyIndicies;
  } else {
    createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.queueFamilyIndexCount = 0;        // optional?
    createInfo.pQueueFamilyIndices   = nullptr;  // optional?
  }

  createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode    = presentMode;
  createInfo.clipped        = VK_TRUE;

  if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapchain) != VK_SUCCESS) {
    throw std::runtime_error("Failed to create swap chain!");
  }

  canRender = true;

  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
  swapchainImages.resize(imageCount);
  vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data());

  swapchainImageFormat = surfaceFomat.format;
  swapchainExtent      = extent;
}

/**
 * @brief My DebugCallback method
 *
 * @param messageSeverity The severity of the message
 * @param messageType The type of message
 * @param pCallbackData Any callback data
 * @param pUserData You can pass user data to the callback, like the
 * HelloTriangle App class
 */
void Core::setup_debug_callback()
{
  if (!enableValidationLayers) {
    return;
  }

  VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
  createInfo.sType                              = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity                    = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;

  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = debug_callback;
  createInfo.pUserData       = nullptr;

  if (create_debug_util_messenger_ext(instance, &createInfo, nullptr, &callback) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug callback");
  }
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Core::load_vulkan_library()
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
bool Core::load_exported_entry_points()
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
#define LoadProcAddress dlsym
#endif

#define VK_EXPORTED_FUNCTION(fun)                                                  \
  if (!(fun = (PFN_##fun)LoadProcAddress(VulkanLibrary, #fun))) {                  \
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
bool Core::load_global_entry_points()
{
#define VK_GLOBAL_LEVEL_FUNCTION(fun)                                                  \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(nullptr, #fun))) {                      \
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
bool Core::load_instance_level_entry_points()
{
#define VK_INSTANCE_LEVEL_FUNCTION(fun)                                                  \
  if (!(fun = (PFN_##fun)vkGetInstanceProcAddr(instance, #fun))) {                       \
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
bool Core::load_device_entry_level_points()
{
#define VK_DEVICE_LEVEL_FUNCTION(fun)                                                    \
  if (!(fun = (PFN_##fun)vkGetDeviceProcAddr(device, #fun))) {                           \
    std::cout << "Could not load instance level function: " << #fun << "!" << std::endl; \
    return false;                                                                        \
  }

#include "vulkan/VulkanFunctions.inl"
  return true;
}

}}  // namespace Rake::Graphics
