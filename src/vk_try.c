#include "vk_try.h"
#include <stdio.h>
#include <stdlib.h>
#include <vulkan/vk_enum_string_helper.h>

void vk_try_impl(VkResult result, const char *vk_function, const char *file,
                 int line, const char *function) {
  if (result != VK_SUCCESS) {
    fprintf(stderr,
            "[\x1B[31mFATAL ERROR\x1B[0m] [%s:%d in '%s'] %s returned %s\n",
            file, line, function, vk_function, string_VkResult(result));
    abort();
  }
}
