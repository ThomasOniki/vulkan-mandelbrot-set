#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define fatal_error(...)                                                       \
  do {                                                                         \
    (void)fprintf(stderr, "[\x1B[31mFATAL ERROR\x1B[0m] [%s:%d in '%s'] ",     \
                  __FILE__, __LINE__, __func__);                               \
    (void)fprintf(stderr, __VA_ARGS__);                                        \
    (void)fputc('\n', stderr);                                                 \
    abort();                                                                   \
  } while (false)
