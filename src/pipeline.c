#include "pipeline.h"
#include "arena.h"
#include "common.h"
#include "device.h"
#include "fatal_error.h"
#include "swapchain.h"
#include "vk_try.h"
#include <stdalign.h>
#include <stdio.h>
#include <string.h>

typedef struct {
  const void *data;
  usize len;
} spv_data_t;

static spv_data_t read_file(const char *path) {
  FILE *fp = fopen(path, "rb");
  if (!fp) {
    fatal_error("Failed to open file %s", path);
  }

  fseek(fp, 0, SEEK_END);
  usize len = (usize)ftell(fp);
  rewind(fp);

  u32 *data = arena_alloc(&MAIN_ARENA, u32, (len + 3) / 4);
  fread(data, len, 1, fp);

  fclose(fp);

  return (spv_data_t){
      .data = data,
      .len = (len + 3) / 4 * 4,
  };
}

static VkShaderModule create_shader_module(const char *spv_path,
                                           device_t *device) {
  spv_data_t spv_data = read_file(spv_path);
  VkShaderModuleCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .codeSize = spv_data.len,
      .pCode = spv_data.data,
  };
  VkShaderModule module;
  vk_try(vkCreateShaderModule(device->device, &info, NULL, &module));
  return module;
}

static void create_shader_modules(pipeline_t *pipeline,
                                  const char *vertex_shader_spv_path,
                                  const char *fragment_shader_spv_path) {
  pipeline->vertex_shader =
      create_shader_module(vertex_shader_spv_path, pipeline->device);

  if (strcmp(vertex_shader_spv_path, fragment_shader_spv_path) == 0) {
    pipeline->fragment_shader = pipeline->vertex_shader;
  } else {
    pipeline->fragment_shader =
        create_shader_module(fragment_shader_spv_path, pipeline->device);
  }
}

static void create_layout(pipeline_t *pipeline) {
  VkPipelineLayoutCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .setLayoutCount = 0,
      .pSetLayouts = NULL,
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL,
  };
  vk_try(vkCreatePipelineLayout(pipeline->device->device, &info, NULL,
                                &pipeline->layout));
}

void create_pipeline(pipeline_t *pipeline, swapchain_t *swapchain,
                     const char *vertex_shader_entry_point,
                     const char *fragment_shader_entry_point) {
  VkPipelineShaderStageCreateInfo stages[2] = {
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .stage = VK_SHADER_STAGE_VERTEX_BIT,
          .module = pipeline->vertex_shader,
          .pName = vertex_shader_entry_point,
          .pSpecializationInfo = NULL,
      },
      {
          .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
          .module = pipeline->fragment_shader,
          .pName = fragment_shader_entry_point,
          .pSpecializationInfo = NULL,
      },
  };

  VkPipelineVertexInputStateCreateInfo vertex_input = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .vertexBindingDescriptionCount = 0,
      .pVertexBindingDescriptions = NULL,
      .vertexAttributeDescriptionCount = 0,
      .pVertexAttributeDescriptions = NULL,
  };
  VkPipelineInputAssemblyStateCreateInfo input_assembly = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
      .primitiveRestartEnable = VK_FALSE,
  };
  VkPipelineViewportStateCreateInfo viewport = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .viewportCount = 1,
      .pViewports = NULL,
      .scissorCount = 1,
      .pScissors = NULL,
  };
  VkPipelineRasterizationStateCreateInfo rasterization = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .depthClampEnable = VK_FALSE,
      .rasterizerDiscardEnable = VK_FALSE,
      .polygonMode = VK_POLYGON_MODE_FILL,
      .cullMode = VK_CULL_MODE_NONE,
      .frontFace = VK_FRONT_FACE_CLOCKWISE,
      .depthBiasEnable = VK_FALSE,
      .depthBiasConstantFactor = 0.0,
      .depthBiasClamp = 0.0,
      .depthBiasSlopeFactor = 0.0,
      .lineWidth = 1.0,
  };
  VkPipelineMultisampleStateCreateInfo multisampling = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
      .sampleShadingEnable = VK_FALSE,
      .minSampleShading = 0.0,
      .pSampleMask = NULL,
      .alphaToCoverageEnable = VK_FALSE,
      .alphaToOneEnable = VK_FALSE,
  };
  VkPipelineColorBlendAttachmentState color_attachment = {
      .blendEnable = VK_FALSE,
      .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .colorBlendOp = VK_BLEND_OP_ADD,
      .srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
      .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
      .alphaBlendOp = VK_BLEND_OP_ADD,
      .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
  };
  VkPipelineColorBlendStateCreateInfo color_blending = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .logicOpEnable = VK_FALSE,
      .logicOp = VK_LOGIC_OP_CLEAR,
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .blendConstants = {1.0, 1.0, 1.0, 1.0},
  };

  VkDynamicState dynamic_states[] = {
      VK_DYNAMIC_STATE_VIEWPORT,
      VK_DYNAMIC_STATE_SCISSOR,
  };
  VkPipelineDynamicStateCreateInfo dynamic_state_info = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .dynamicStateCount = 2,
      .pDynamicStates = dynamic_states,
  };

  VkGraphicsPipelineCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .stageCount = 2,
      .pStages = stages,
      .pVertexInputState = &vertex_input,
      .pInputAssemblyState = &input_assembly,
      .pTessellationState = NULL,
      .pViewportState = &viewport,
      .pRasterizationState = &rasterization,
      .pMultisampleState = &multisampling,
      .pDepthStencilState = NULL,
      .pColorBlendState = &color_blending,
      .pDynamicState = &dynamic_state_info,
      .layout = pipeline->layout,
      .renderPass = swapchain->render_pass,
      .subpass = 0,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = -1,
  };

  vk_try(vkCreateGraphicsPipelines(pipeline->device->device, VK_NULL_HANDLE, 1,
                                   &info, NULL, &pipeline->pipeline));
}

void pipeline_init(pipeline_t *pipeline, device_t *device,
                   swapchain_t *swapchain, const char *vertex_shader_spv_path,
                   const char *vertex_shader_entry_point,
                   const char *fragment_shader_spv_path,
                   const char *fragment_shader_entry_point) {
  pipeline->device = device;

  create_shader_modules(pipeline, vertex_shader_spv_path,
                        fragment_shader_spv_path);
  create_layout(pipeline);
  create_pipeline(pipeline, swapchain, vertex_shader_entry_point,
                  fragment_shader_entry_point);
}

void pipeline_cleanup(pipeline_t *pipeline) {
  vkDestroyPipeline(pipeline->device->device, pipeline->pipeline, NULL);
  vkDestroyPipelineLayout(pipeline->device->device, pipeline->layout, NULL);

  if (pipeline->vertex_shader != pipeline->fragment_shader) {
    vkDestroyShaderModule(pipeline->device->device, pipeline->vertex_shader,
                          NULL);
  }
  vkDestroyShaderModule(pipeline->device->device, pipeline->fragment_shader,
                        NULL);
}
