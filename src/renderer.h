#pragma once

#include "common.h"

enum { MAX_FRAMES_IN_FLIGHT = 2 };

struct renderer_t {
  device_t *device;
  swapchain_t *swapchain;
  command_buffer_t *command_buffers;
  VkSemaphore *render_finished_semaphores;
  VkFence render_finished_fences[MAX_FRAMES_IN_FLIGHT];
  VkSemaphore image_acquired_semaphores[MAX_FRAMES_IN_FLIGHT];
  u32 frame_index;
};

void renderer_init(renderer_t *renderer, device_t *device,
                   swapchain_t *swapchain);
void renderer_cleanup(renderer_t *renderer);

void renderer_draw_frame(renderer_t *renderer);
