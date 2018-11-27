
/**
 * @brief These functions are always exposed by the vulkan library
 *
 */
#if !defined(VK_EXPORTED_FUNCTION)
#define VK_EXPORTED_FUNCTION(fun)
#endif

VK_EXPORTED_FUNCTION(vkGetInstanceProcAddr)

#undef VK_EXPORTED_FUNCTION

/**
 * @brief They allow checking what instance extensions are available
 * and allow creations of a Vulkan instance
 *
 */
#if !defined(VK_GLOBAL_LEVEL_FUNCTION)
#define VK_GLOBAL_LEVEL_FUNCTION(fun)
#endif

VK_GLOBAL_LEVEL_FUNCTION(vkCreateInstance)
VK_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceExtensionProperties)
VK_GLOBAL_LEVEL_FUNCTION(vkEnumerateInstanceLayerProperties)

#undef VK_GLOBAL_LEVEL_FUNCTION

/**
 * @brief Allow for device queries and creation.
 *
 *
 */
#if !defined(VK_INSTANCE_LEVEL_FUNCTION)
#define VK_INSTANCE_LEVEL_FUNCTION(fun)
#endif

VK_INSTANCE_LEVEL_FUNCTION(vkEnumeratePhysicalDevices)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceFeatures)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceQueueFamilyProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateDevice)
VK_INSTANCE_LEVEL_FUNCTION(vkGetDeviceProcAddr)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyInstance)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroySurfaceKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateXcbSurfaceKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSurfaceSupportKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkEnumerateDeviceExtensionProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSurfaceCapabilitiesKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSurfaceFormatsKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceSurfacePresentModesKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateSwapchainKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroySwapchainKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkGetSwapchainImagesKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateImageView)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyImageView)
VK_INSTANCE_LEVEL_FUNCTION(vkAcquireNextImageKHR)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateShaderModule)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyShaderModule)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateRenderPass)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyRenderPass)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateGraphicsPipelines)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyPipeline)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyFramebuffer)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyCommandPool)
VK_INSTANCE_LEVEL_FUNCTION(vkBeginCommandBuffer)
VK_INSTANCE_LEVEL_FUNCTION(vkCmdBindPipeline)
VK_INSTANCE_LEVEL_FUNCTION(vkFreeCommandBuffers)
VK_INSTANCE_LEVEL_FUNCTION(vkDestroyFence)
VK_INSTANCE_LEVEL_FUNCTION(vkWaitForFences)
VK_INSTANCE_LEVEL_FUNCTION(vkResetFences)
VK_INSTANCE_LEVEL_FUNCTION(vkCreateFence)
VK_INSTANCE_LEVEL_FUNCTION(vkGetFenceStatus)
VK_INSTANCE_LEVEL_FUNCTION(vkGetPhysicalDeviceMemoryProperties)
VK_INSTANCE_LEVEL_FUNCTION(vkQueueWaitIdle)
VK_INSTANCE_LEVEL_FUNCTION(vkCmdBindIndexBuffer)
VK_INSTANCE_LEVEL_FUNCTION(vkCmdDrawIndexed)

#undef VK_INSTANCE_LEVEL_FUNCTION

/**
 * @brief Used mainly for drawing
 *
 */
#if !defined(VK_DEVICE_LEVEL_FUNCTION)
#define VK_DEVICE_LEVEL_FUNCTION(fun)
#endif

VK_DEVICE_LEVEL_FUNCTION(vkGetDeviceQueue)
VK_DEVICE_LEVEL_FUNCTION(vkDeviceWaitIdle)
VK_DEVICE_LEVEL_FUNCTION(vkDestroyDevice)

VK_DEVICE_LEVEL_FUNCTION(vkCreatePipelineLayout)
VK_DEVICE_LEVEL_FUNCTION(vkDestroyPipelineLayout)
VK_DEVICE_LEVEL_FUNCTION(vkCreateFramebuffer)
VK_DEVICE_LEVEL_FUNCTION(vkCreateCommandPool)
VK_DEVICE_LEVEL_FUNCTION(vkAllocateCommandBuffers)
VK_DEVICE_LEVEL_FUNCTION(vkCmdBeginRenderPass)
VK_DEVICE_LEVEL_FUNCTION(vkCmdDraw)
VK_DEVICE_LEVEL_FUNCTION(vkCmdEndRenderPass)
VK_DEVICE_LEVEL_FUNCTION(vkEndCommandBuffer)
VK_DEVICE_LEVEL_FUNCTION(vkCreateSemaphore)
VK_DEVICE_LEVEL_FUNCTION(vkDestroySemaphore)
VK_DEVICE_LEVEL_FUNCTION(vkQueueSubmit)
VK_DEVICE_LEVEL_FUNCTION(vkQueuePresentKHR)
VK_DEVICE_LEVEL_FUNCTION(vkCreateBuffer)
VK_DEVICE_LEVEL_FUNCTION(vkDestroyBuffer)
VK_DEVICE_LEVEL_FUNCTION(vkGetBufferMemoryRequirements)
VK_DEVICE_LEVEL_FUNCTION(vkFreeMemory)
VK_DEVICE_LEVEL_FUNCTION(vkAllocateMemory)
VK_DEVICE_LEVEL_FUNCTION(vkBindBufferMemory)
VK_DEVICE_LEVEL_FUNCTION(vkMapMemory)
VK_DEVICE_LEVEL_FUNCTION(vkUnmapMemory)
VK_DEVICE_LEVEL_FUNCTION(vkCmdBindVertexBuffers)
VK_DEVICE_LEVEL_FUNCTION(vkCmdCopyBuffer)
VK_DEVICE_LEVEL_FUNCTION(vkDestroyDescriptorSetLayout)
VK_DEVICE_LEVEL_FUNCTION(vkCreateDescriptorSetLayout)

#undef VK_DEVICE_LEVEL_FUNCTION
