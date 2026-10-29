#include "renderer.h"
#include "arena.h"
#include "command_buffer.h"
#include "device.h"
#include "swapchain.h"
#include "vk_try.h"

void renderer_init(renderer_t *renderer, device_t *device,
                   swapchain_t *swapchain) {
  renderer->device = device;
  renderer->swapchain = swapchain;

  renderer->command_buffers =
      arena_alloc(&MAIN_ARENA, command_buffer_t, swapchain->image_count);

  //  Don't start recording yet so all commands buffers can be allocated at
  //  once.
  for (u32 i = 0; i < swapchain->image_count; ++i) {
    command_buffer_init(&renderer->command_buffers[i], device);
  }

  VkSemaphoreCreateInfo semaphore_info = {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
  };
  VkFenceCreateInfo fence_info = {
      .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_FENCE_CREATE_SIGNALED_BIT,
  };
  for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vk_try(vkCreateSemaphore(device->device, &semaphore_info, NULL,
                             &renderer->image_acquired_semaphores[i]));
    vk_try(vkCreateFence(device->device, &fence_info, NULL,
                         &renderer->render_finished_fences[i]));
  }
  renderer->render_finished_semaphores =
      arena_alloc(&MAIN_ARENA, VkSemaphore, swapchain->image_count);
  for (u32 i = 0; i < swapchain->image_count; ++i) {
    vk_try(vkCreateSemaphore(device->device, &semaphore_info, NULL,
                             &renderer->render_finished_semaphores[i]));
  }
}

void renderer_cleanup(renderer_t *renderer) {
  for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
    vkDestroySemaphore(renderer->device->device,
                       renderer->image_acquired_semaphores[i], NULL);
    vkDestroyFence(renderer->device->device,
                   renderer->render_finished_fences[i], NULL);
  }
  for (u32 i = 0; i < renderer->swapchain->image_count; ++i) {
    vkDestroySemaphore(renderer->device->device,
                       renderer->render_finished_semaphores[i], NULL);
  }
}

void renderer_draw_frame(renderer_t *renderer) {
  renderer->frame_index = (renderer->frame_index + 1) % MAX_FRAMES_IN_FLIGHT;

  vkWaitForFences(renderer->device->device, 1,
                  &renderer->render_finished_fences[renderer->frame_index],
                  VK_TRUE, UINT64_MAX);
  vkResetFences(renderer->device->device, 1,
                &renderer->render_finished_fences[renderer->frame_index]);

  u32 image_index = 0;
  vk_try(vkAcquireNextImageKHR(
      renderer->device->device, renderer->swapchain->swapchain, UINT64_MAX,
      renderer->image_acquired_semaphores[renderer->frame_index],
      VK_NULL_HANDLE, &image_index));

  VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  VkSubmitInfo draw_info = {
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .pNext = NULL,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores =
          &renderer->image_acquired_semaphores[renderer->frame_index],
      .pWaitDstStageMask = &wait_stage,
      .commandBufferCount = 1,
      .pCommandBuffers = &renderer->command_buffers[image_index].buffer,
      .signalSemaphoreCount = 1,
      .pSignalSemaphores = &renderer->render_finished_semaphores[image_index],
  };
  vkQueueSubmit(renderer->device->graphics_queue, 1, &draw_info,
                renderer->render_finished_fences[renderer->frame_index]);

  VkPresentInfoKHR present_info = {
      .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
      .pNext = NULL,
      .waitSemaphoreCount = 1,
      .pWaitSemaphores = &renderer->render_finished_semaphores[image_index],
      .swapchainCount = 1,
      .pSwapchains = &renderer->swapchain->swapchain,
      .pImageIndices = &image_index,
      .pResults = NULL,
  };
  vk_try(vkQueuePresentKHR(renderer->device->present_queue, &present_info));
}
