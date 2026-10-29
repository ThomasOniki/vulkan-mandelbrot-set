#pragma once

#include "common.h"
#include <vulkan/vulkan_core.h>

struct pipeline_t {
  device_t *device;
  VkShaderModule vertex_shader;
  VkShaderModule fragment_shader;
  VkPipelineLayout layout;
  VkPipeline pipeline;
};

void pipeline_init(pipeline_t *pipeline, device_t *device,
                   swapchain_t *swapchain, const char *vertex_shader_spv_path,
                   const char *vertex_shader_entry_point,
                   const char *fragment_shader_spv_path,
                   const char *fragment_shader_entry_point);
void pipeline_cleanup(pipeline_t *pipeline);
