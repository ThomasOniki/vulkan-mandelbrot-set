#pragma once

#include "common.h"
#include <vulkan/vulkan_core.h>

struct swapchain_t {
  device_t *device;
  window_t *window;
  VkSwapchainKHR swapchain;
  VkFormat format;
  u32 image_count;
  VkImage *images;
  VkImageView *image_views;
  VkFramebuffer *frame_buffers;
  VkRenderPass render_pass;
};

void swapchain_init(swapchain_t *swapchain, device_t *device, window_t *window);
void swapchain_cleanup(swapchain_t *swapchain);
