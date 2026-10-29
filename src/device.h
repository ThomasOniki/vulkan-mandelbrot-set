#pragma once

#include "common.h"
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

typedef struct {
#ifdef ENABLE_VALIDATION_LAYERS
  PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT;
  PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT;
#endif
} vk_functions_t;

struct device_t {
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice gpu;
  VkDevice device;
  u32 queue_family_indices[2]; // [0] = graphics, [1] = presentation
  u32 unique_queue_family_count;
  VkQueue graphics_queue;
  VkQueue present_queue;
  // graphics_queue == present_queue ?
  // VK_SHARING_MODE_EXCLUSIVE : VK_SHARING_MODE_CONCURRENT
  VkSharingMode queue_sharing_mode;
  vk_functions_t functions;
  window_t **windows;
  usize window_count;
  VkCommandPool command_pool;
};

void device_init(device_t *device, window_t *windows[], usize window_count);
void device_cleanup(device_t *device);
