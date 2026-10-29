#pragma once

#include <vulkan/vulkan_core.h>

void vk_try_impl(VkResult result, const char *vk_function, const char *file,
                 int line, const char *function);

#define vk_try(func) vk_try_impl(func, #func, __FILE__, __LINE__, __func__)
