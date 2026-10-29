#include "swapchain.h"
#include "arena.h"
#include "device.h"
#include "fatal_error.h"
#include "vk_try.h"
#include "window.h"
#include <stddef.h>
#include <stdint.h>

static VkSurfaceFormatKHR pick_surface_format(device_t *device,
                                              window_t *window) {
  u32 format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device->gpu, window->surface,
                                       &format_count, NULL);
  if (format_count == 0) {
    fatal_error("No surface formats available.");
  }
  VkSurfaceFormatKHR *formats =
      arena_alloc(&MAIN_ARENA, VkSurfaceFormatKHR, format_count);
  vkGetPhysicalDeviceSurfaceFormatsKHR(device->gpu, window->surface,
                                       &format_count, formats);

  for (usize i = 0; i < format_count; ++i) {
    if ((formats[i].format == VK_FORMAT_R8G8B8A8_SRGB ||
         formats[i].format == VK_FORMAT_B8G8R8A8_SRGB) &&
        formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return formats[i];
    }
  }
  return formats[0];
}

static VkPresentModeKHR pick_present_mode(device_t *device, window_t *window) {
  u32 mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device->gpu, window->surface,
                                            &mode_count, NULL);
  VkPresentModeKHR *modes =
      arena_alloc(&MAIN_ARENA, VkPresentModeKHR, mode_count);
  vkGetPhysicalDeviceSurfacePresentModesKHR(device->gpu, window->surface,
                                            &mode_count, modes);
  for (usize i = 0; i < mode_count; ++i) {
    if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
      return VK_PRESENT_MODE_MAILBOX_KHR;
    }
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

static u32 pick_min_image_count(const VkSurfaceCapabilitiesKHR *capabilities) {
  u32 min_image_count =
      capabilities->minImageCount > 3 ? capabilities->minImageCount : 3;
  if (capabilities->maxImageCount > 0 &&
      capabilities->maxImageCount < min_image_count) {
    min_image_count = capabilities->maxImageCount;
  }
  return min_image_count;
}

static u32 clamp_u32(u32 num, u32 min, u32 max) {
  if (num < min) {
    num = min;
  } else if (num > max) {
    num = max;
  }
  return num;
}

VkExtent2D pick_extent(const VkSurfaceCapabilitiesKHR *capabilities,
                       const window_t *window) {
  if (capabilities->currentExtent.width != UINT32_MAX) {
    return capabilities->currentExtent;
  }
  return (VkExtent2D){
      .width = clamp_u32(window->frame_buffer_width,
                         capabilities->minImageExtent.width,
                         capabilities->maxImageExtent.width),
      .height = clamp_u32(window->frame_buffer_height,
                          capabilities->minImageExtent.height,
                          capabilities->maxImageExtent.height),
  };
}

void create_swapchain(swapchain_t *swapchain, window_t *window) {
  VkSurfaceFormatKHR surface_format =
      pick_surface_format(swapchain->device, window);

  swapchain->format = surface_format.format;

  VkSurfaceCapabilitiesKHR capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(swapchain->device->gpu,
                                            window->surface, &capabilities);

  VkSwapchainCreateInfoKHR info = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .surface = window->surface,
      .minImageCount = pick_min_image_count(&capabilities),
      .imageFormat = surface_format.format,
      .imageColorSpace = surface_format.colorSpace,
      .imageExtent = pick_extent(&capabilities, window),
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
      .imageSharingMode = swapchain->device->queue_sharing_mode,
      .queueFamilyIndexCount = swapchain->device->unique_queue_family_count,
      .pQueueFamilyIndices = swapchain->device->queue_family_indices,
      .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = pick_present_mode(swapchain->device, window),
      .clipped = VK_TRUE,
      .oldSwapchain = NULL,
  };
  vk_try(vkCreateSwapchainKHR(swapchain->device->device, &info, NULL,
                              &swapchain->swapchain));
}

void retrieve_images(swapchain_t *swapchain) {
  vk_try(vkGetSwapchainImagesKHR(swapchain->device->device,
                                 swapchain->swapchain, &swapchain->image_count,
                                 NULL));
  swapchain->images = arena_alloc(&MAIN_ARENA, VkImage, swapchain->image_count);
  vk_try(vkGetSwapchainImagesKHR(swapchain->device->device,
                                 swapchain->swapchain, &swapchain->image_count,
                                 swapchain->images));
}

void create_image_views(swapchain_t *swapchain) {
  VkComponentMapping swizzle = {
      .r = VK_COMPONENT_SWIZZLE_R,
      .g = VK_COMPONENT_SWIZZLE_G,
      .b = VK_COMPONENT_SWIZZLE_B,
      .a = VK_COMPONENT_SWIZZLE_A,
  };
  VkImageSubresourceRange subresource_range = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
  };
  VkImageViewCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .image = NULL,
      .viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY,
      .format = swapchain->format,
      .components = swizzle,
      .subresourceRange = subresource_range,
  };
  swapchain->image_views =
      arena_alloc(&MAIN_ARENA, VkImageView, swapchain->image_count);

  for (u32 i = 0; i < swapchain->image_count; ++i) {
    info.image = swapchain->images[i];
    vk_try(vkCreateImageView(swapchain->device->device, &info, NULL,
                             &swapchain->image_views[i]));
  }
}

static void create_render_pass(swapchain_t *swapchain) {
  VkAttachmentDescription color_attachment = {
      .flags = 0,
      .format = swapchain->format,
      .samples = VK_SAMPLE_COUNT_1_BIT,
      .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
      .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
      .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
      .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
      .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
  };
  VkAttachmentReference color_attachment_ref = {
      .attachment = 0,
      .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
  };
  VkSubpassDescription subpass_description = {
      .flags = 0,
      .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
      .inputAttachmentCount = 0,
      .pInputAttachments = NULL,
      .colorAttachmentCount = 1,
      .pColorAttachments = &color_attachment_ref,
      .pResolveAttachments = NULL,
      .pDepthStencilAttachment = NULL,
      .preserveAttachmentCount = 0,
      .pPreserveAttachments = NULL,
  };
  VkRenderPassCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .attachmentCount = 1,
      .pAttachments = &color_attachment,
      .subpassCount = 1,
      .pSubpasses = &subpass_description,
      .dependencyCount = 0,
      .pDependencies = NULL,
  };
  vk_try(vkCreateRenderPass(swapchain->device->device, &info, NULL,
                            &swapchain->render_pass));
}

void create_frame_buffers(swapchain_t *swapchain) {
  VkFramebufferCreateInfo info = {
      .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .renderPass = swapchain->render_pass,
      .attachmentCount = 1,
      .pAttachments = NULL,
      .width = swapchain->window->frame_buffer_width,
      .height = swapchain->window->frame_buffer_height,
      .layers = 1,
  };
  swapchain->frame_buffers =
      arena_alloc(&MAIN_ARENA, VkFramebuffer, swapchain->image_count);
  for (u32 i = 0; i < swapchain->image_count; ++i) {
    info.pAttachments = &swapchain->image_views[i];
    vk_try(vkCreateFramebuffer(swapchain->device->device, &info, NULL,
                               &swapchain->frame_buffers[i]));
  }
}

void swapchain_init(swapchain_t *swapchain, device_t *device,
                    window_t *window) {
  swapchain->device = device;
  swapchain->window = window;
  create_swapchain(swapchain, window);
  create_render_pass(swapchain);
  retrieve_images(swapchain);
  create_image_views(swapchain);
  create_frame_buffers(swapchain);
}

void swapchain_cleanup(swapchain_t *swapchain) {
  for (u32 i = 0; i < swapchain->image_count; ++i) {
    vkDestroyFramebuffer(swapchain->device->device, swapchain->frame_buffers[i],
                         NULL);
    vkDestroyImageView(swapchain->device->device, swapchain->image_views[i],
                       NULL);
  }
  vkDestroyRenderPass(swapchain->device->device, swapchain->render_pass, NULL);
  vkDestroySwapchainKHR(swapchain->device->device, swapchain->swapchain, NULL);
}
