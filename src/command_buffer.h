#pragma once

#include "common.h"
#include <stdalign.h>
#include <vulkan/vulkan_core.h>

struct command_buffer_t {
  device_t *device;
  VkCommandBuffer buffer;
};

void command_buffer_init(command_buffer_t *command_buffer, device_t *device);
void command_buffer_begin_recording(command_buffer_t *command_buffer);

void command_buffer_end_recording(command_buffer_t *command_buffer);

void command_buffer_begin_rendering(command_buffer_t *command_buffer,
                                    swapchain_t *swapchain, u32 image_index);
void command_buffer_end_rendering(command_buffer_t *command_buffer);

void command_buffer_bind_pipeline(command_buffer_t *command_buffer,
                                  pipeline_t *pipeline);
