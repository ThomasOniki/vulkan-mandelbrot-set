#include "device.h"
#include "arena.h"
#include "array.h"
#include "fatal_error.h"
#include "vk_try.h"
#include "window.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static VKAPI_ATTR VKAPI_CALL VkBool32
debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
               VkDebugUtilsMessageTypeFlagsEXT messageTypes,
               const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
               void *pUserData) {
  printf("[DEBUG CALLBACK] %s\n", pCallbackData->pMessage);
  return VK_TRUE;
}

VkDebugUtilsMessengerCreateInfoEXT *get_debug_messenger_info() {
  static VkDebugUtilsMessengerCreateInfoEXT info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .pNext = NULL,
      .flags = 0,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                     VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
      .pfnUserCallback = debug_callback,
      NULL,
  };
  return &info;
}

static arr_t(const char *) get_instance_extensions() {
  u32 glfw_extension_count;
  const char **glfw_extensions =
      glfwGetRequiredInstanceExtensions(&glfw_extension_count);

  const char **extensions = NULL;

  arr_reserve(extensions, glfw_extension_count + 4);
  arr_push_elems(extensions, glfw_extensions, glfw_extension_count);

#ifndef NDEBUG
  arr_push(extensions, "VK_EXT_debug_utils");
#endif
#ifdef __APPLE__
  arr_push(extensions, "VK_KHR_portability_enumeration");
  arr_push(extensions, "VK_KHR_get_physical_device_properties2");
#endif

  return extensions;
}

static void create_instance(device_t *device) {
#ifdef ENABLE_VALIDATION_LAYERS
  const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
#else
  const char *validation_layers[] = {};
#endif

  VkApplicationInfo app_info = {
      .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
      .pNext = NULL,
      .pApplicationName = "vulkan-mandelbrot-set",
      .applicationVersion = VK_MAKE_VERSION(0, 0, 0),
      .pEngineName = "no engine",
      .engineVersion = VK_MAKE_VERSION(0, 0, 0),
      .apiVersion = VK_API_VERSION_1_4,
  };

  arr_t(const char *) extensions = get_instance_extensions();

  VkInstanceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .pApplicationInfo = &app_info,
      .enabledLayerCount =
          sizeof(validation_layers) / sizeof(validation_layers[0]),
      .ppEnabledLayerNames = validation_layers,
      .enabledExtensionCount = arr_len(extensions),
      .ppEnabledExtensionNames = extensions,
  };
#ifdef __APPLE__
  info.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
  vk_try(vkCreateInstance(&info, NULL, &device->instance));
}

#define LOAD_INSTANCE_FUNCTION(device, func)                                   \
  do {                                                                         \
    (device)->functions.func =                                                 \
        (PFN_##func)vkGetInstanceProcAddr((device)->instance, #func);          \
    if (!(device)->functions.func) {                                           \
      fatal_error("failed to load instance function %s", #func);               \
    }                                                                          \
  } while (false)

static void load_instance_functions(device_t *device) {
#ifdef ENABLE_VALIDATION_LAYERS
  LOAD_INSTANCE_FUNCTION(device, vkCreateDebugUtilsMessengerEXT);
  LOAD_INSTANCE_FUNCTION(device, vkDestroyDebugUtilsMessengerEXT);
#endif
}

static void create_debug_messenger(device_t *device) {
#ifdef ENABLE_VALIDATION_LAYERS
  vk_try(device->functions.vkCreateDebugUtilsMessengerEXT(
      device->instance, get_debug_messenger_info(), NULL,
      &device->debug_messenger));
#endif
}

static void create_surfaces(device_t *device) {
  for (usize i = 0; i < device->window_count; ++i) {
    vk_try(glfwCreateWindowSurface(device->instance, device->windows[i]->window,
                                   NULL, &device->windows[i]->surface));
  }
}

static void pick_gpu(device_t *device) {
  u32 gpu_count;
  vk_try(vkEnumeratePhysicalDevices(device->instance, &gpu_count, NULL));
  VkPhysicalDevice *gpus =
      arena_alloc(&MAIN_ARENA, VkPhysicalDevice, gpu_count);
  vk_try(vkEnumeratePhysicalDevices(device->instance, &gpu_count, gpus));
  if (gpu_count == 0) {
    fatal_error("No gpu found.");
  }
  // TODO: thomas - pick the best gpu, not the first
  device->gpu = gpus[0];
}

static void get_queue_family_indices(device_t *device) {
  u32 family_count;
  vkGetPhysicalDeviceQueueFamilyProperties(device->gpu, &family_count, NULL);
  VkQueueFamilyProperties *properties =
      arena_alloc(&MAIN_ARENA, VkQueueFamilyProperties, family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(device->gpu, &family_count,
                                           properties);

  bool graphics_queue_found = false;
  bool present_queue_found = false;
  for (u32 i = 0; i < family_count; ++i) {
    if (properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT &&
        !graphics_queue_found) {
      device->queue_family_indices[0] = i;
      graphics_queue_found = true;
    }
    VkBool32 can_present = VK_TRUE;
    for (usize i = 0; i < device->window_count; ++i) {
      vkGetPhysicalDeviceSurfaceSupportKHR(
          device->gpu, i, device->windows[i]->surface, &can_present);
      if (!can_present) {
        break;
      }
    }
    if (can_present == VK_TRUE && !present_queue_found) {
      device->queue_family_indices[1] = i;
      present_queue_found = true;
    }
  }
  if (!graphics_queue_found) {
    fatal_error("GPU graphics queue not found.");
  }
  if (!present_queue_found) {
    fatal_error("GPU presentation queue not found.");
  }
  if (device->queue_family_indices[0] == device->queue_family_indices[1]) {
    device->unique_queue_family_count = 1;
    device->queue_sharing_mode = VK_SHARING_MODE_EXCLUSIVE;
  } else {
    device->unique_queue_family_count = 2;
    device->queue_sharing_mode = VK_SHARING_MODE_CONCURRENT;
  }
}

static arr_t(const char *) get_device_extensions() {
  arr_t(const char *) extensions = NULL;
  arr_reserve(extensions, 8);
#ifdef __APPLE__
  arr_push(extensions, "VK_KHR_portability_subset");
#endif
  arr_push(extensions, "VK_KHR_swapchain");
  return extensions;
}

static void create_device(device_t *device) {
  float priority = 1;
  VkDeviceQueueCreateInfo queue_info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = device->queue_family_indices[0],
      .queueCount = 1,
      .pQueuePriorities = &priority,
  };
  VkDeviceQueueCreateInfo queue_infos[] = {queue_info, queue_info};
  queue_infos[1].queueFamilyIndex = device->queue_family_indices[1];

  arr_t(const char *) extensions = get_device_extensions();

  VkDeviceCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueCreateInfoCount = device->unique_queue_family_count,
      .pQueueCreateInfos = queue_infos,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
      .enabledExtensionCount = arr_len(extensions),
      .ppEnabledExtensionNames = extensions,
      .pEnabledFeatures = NULL,
  };
  vk_try(vkCreateDevice(device->gpu, &info, NULL, &device->device));
}

static void retrieve_queues(device_t *device) {
  vkGetDeviceQueue(device->device, device->queue_family_indices[0], 0,
                   &device->graphics_queue);
  if (device->unique_queue_family_count == 1) {
    device->present_queue = device->graphics_queue;
  } else {
    vkGetDeviceQueue(device->device, device->queue_family_indices[1], 0,
                     &device->present_queue);
  }
}

static void create_command_pool(device_t *device) {
  VkCommandPoolCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = device->queue_family_indices[0],
  };
  vk_try(
      vkCreateCommandPool(device->device, &info, NULL, &device->command_pool));
}

void device_init(device_t *device, window_t *windows[], usize window_count) {
  device->windows = windows;
  device->window_count = window_count;
  create_instance(device);
  load_instance_functions(device);
  create_debug_messenger(device);
  create_surfaces(device);
  pick_gpu(device);
  get_queue_family_indices(device);
  create_device(device);
  retrieve_queues(device);
  create_command_pool(device);
}

void device_cleanup(device_t *device) {
  vkDestroyCommandPool(device->device, device->command_pool, NULL);
  vkDestroyDevice(device->device, NULL);
#ifdef ENABLE_VALIDATION_LAYERS
  device->functions.vkDestroyDebugUtilsMessengerEXT(
      device->instance, device->debug_messenger, NULL);
#endif
  for (usize i = 0; i < device->window_count; ++i) {
    vkDestroySurfaceKHR(device->instance, device->windows[i]->surface, NULL);
  }
  vkDestroyInstance(device->instance, NULL);
}
