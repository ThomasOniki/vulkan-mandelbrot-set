#include "arena.h"
#include "command_buffer.h"
#include "device.h"
#include "pipeline.h"
#include "renderer.h"
#include "swapchain.h"
#include "window.h"
#include <stdbool.h>
#include <stddef.h>

int main() {
  window_t window;
  window_open(&window, 800, 600, "mandelbrot-vk");

  window_t *windows[] = {&window};
  const usize window_count = 1;

  device_t device;
  device_init(&device, windows, window_count);
  swapchain_t swapchain;
  swapchain_init(&swapchain, &device, &window);
  pipeline_t pipeline;
  pipeline_init(&pipeline, &device, &swapchain, "shaders/set.vert.spv", "main",
                "shaders/set.frag.spv", "main");
  renderer_t renderer;
  renderer_init(&renderer, &device, &swapchain);

  for (u32 i = 0; i < swapchain.image_count; ++i) {
    command_buffer_begin_recording(&renderer.command_buffers[i]);
    command_buffer_begin_rendering(&renderer.command_buffers[i], &swapchain, i);

    command_buffer_bind_pipeline(&renderer.command_buffers[i], &pipeline);

    vkCmdDraw(renderer.command_buffers[i].buffer, 3, 1, 0, 0);

    command_buffer_end_rendering(&renderer.command_buffers[i]);
    command_buffer_end_recording(&renderer.command_buffers[i]);
  }

  u32 frame_index = 0;

  while (!glfwWindowShouldClose(window.window)) {
    glfwPollEvents();
    renderer_draw_frame(&renderer);
  }

  vkDeviceWaitIdle(device.device);

  renderer_cleanup(&renderer);
  pipeline_cleanup(&pipeline);
  swapchain_cleanup(&swapchain);
  device_cleanup(&device);
  window_close(&window);
}
