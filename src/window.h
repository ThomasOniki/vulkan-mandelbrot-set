#pragma once

#include "common.h"
#include <stdalign.h>
#include <vulkan/vulkan_core.h>

#include <GLFW/glfw3.h>

struct window_t {
  alignas(32) GLFWwindow *window;
  VkSurfaceKHR surface;
  u32 frame_buffer_width;
  u32 frame_buffer_height;
};

void window_open(window_t *window, int width, int height, const char *title);
void window_close(window_t *window);
