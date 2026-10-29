#include "command_buffer.h"
#include "arena.h"
#include "array.h"
#include "device.h"
#include "pipeline.h"
#include "swapchain.h"
#include "vk_try.h"
#include "window.h"

static arr_t(command_buffer_t *) * get_cached_command_buffers() {
  static _Thread_local arr_t(command_buffer_t *) cache = NULL;
  return &cache;
}

void command_buffer_init(command_buffer_t *command_buffer, device_t *device) {
  command_buffer->device = device;
  command_buffer->buffer = VK_NULL_HANDLE;
  arr_push(*get_cached_command_buffers(), command_buffer);
}

void command_buffer_begin_recording(command_buffer_t *command_buffer) {
  arr_t(command_buffer_t *) cache = *get_cached_command_buffers();
  if (arr_len(cache) > 0) {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = NULL,
        .commandPool = command_buffer->device->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = arr_len(cache),
    };
    VkCommandBuffer *buffers =
        arena_alloc(&MAIN_ARENA, VkCommandBuffer, arr_len(cache));

    vk_try(vkAllocateCommandBuffers(command_buffer->device->device, &alloc_info,
                                    buffers));
    for (u32 i = 0; i < arr_len(cache); ++i) {
      cache[i]->buffer = buffers[i];
    }
    arr_clear(*get_cached_command_buffers());
  }

  VkCommandBufferBeginInfo info = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .pNext = NULL,
      .flags = 0,
      .pInheritanceInfo = NULL,
  };
  vk_try(vkBeginCommandBuffer(command_buffer->buffer, &info));
}

void command_buffer_end_recording(command_buffer_t *command_buffer) {
  vk_try(vkEndCommandBuffer(command_buffer->buffer));
}

void command_buffer_begin_rendering(command_buffer_t *command_buffer,
                                    swapchain_t *swapchain, u32 image_index) {
  VkClearValue clear_value = {
      .color = {.uint32 = {255, 255, 255, 255}},
  };

  VkRenderPassBeginInfo info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
      .pNext = NULL,
      .renderPass = swapchain->render_pass,
      .framebuffer = swapchain->frame_buffers[image_index],
      .renderArea =
          {
              .offset = {0, 0},
              .extent = {swapchain->window->frame_buffer_width,
                         swapchain->window->frame_buffer_height},
          },
      .clearValueCount = 1,
      .pClearValues = &clear_value,
  };
  vkCmdBeginRenderPass(command_buffer->buffer, &info,
                       VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {
      .x = 0,
      .y = 0,
      .width = swapchain->window->frame_buffer_width,
      .height = swapchain->window->frame_buffer_height,
      .minDepth = 0,
      .maxDepth = 1,
  };
  vkCmdSetViewport(command_buffer->buffer, 0, 1, &viewport);

  VkRect2D scissor = {
      .offset = {0, 0},
      .extent = {.width = swapchain->window->frame_buffer_width,
                 .height = swapchain->window->frame_buffer_height},
  };
  vkCmdSetScissor(command_buffer->buffer, 0, 1, &scissor);
}

void command_buffer_end_rendering(command_buffer_t *command_buffer) {
  vkCmdEndRenderPass(command_buffer->buffer);
}

void command_buffer_bind_pipeline(command_buffer_t *command_buffer,
                                  pipeline_t *pipeline) {
  vkCmdBindPipeline(command_buffer->buffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipeline->pipeline);
}
