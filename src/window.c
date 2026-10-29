#include "window.h"
#include "GLFW/glfw3.h"
#include "common.h"
#include "fatal_error.h"
#include <pthread.h>

static pthread_mutex_t *get_mutex() {
  static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
  return &mutex;
}
static u32 *get_active_window_count() {
  static u32 count = 0;
  return &count;
}

static void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  window_t *user = glfwGetWindowUserPointer(window);
  user->frame_buffer_width = width;
  user->frame_buffer_height = width;
}

void window_open(window_t *window, int width, int height, const char *title) {
  pthread_mutex_lock(get_mutex());
  if (*get_active_window_count() == 0) {
    if (glfwInit() == GLFW_FALSE) {
      fatal_error("Failed to initialize glfw.");
    }
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  }
  *get_active_window_count() += 1;
  pthread_mutex_unlock(get_mutex());

  window->window = glfwCreateWindow(width, height, title, NULL, NULL);
  if (window->window == NULL) {
    fatal_error("Failed to create GLFWwindow.");
  }

  int fbo_width = 0;
  int fbo_height = 0;
  glfwGetFramebufferSize(window->window, &fbo_width, &fbo_height);
  window->frame_buffer_width = fbo_width;
  window->frame_buffer_height = fbo_height;

  glfwSetWindowUserPointer(window->window, window);
  glfwSetFramebufferSizeCallback(window->window, framebuffer_size_callback);
}

void window_close(window_t *window) {
  glfwDestroyWindow(window->window);

  pthread_mutex_lock(get_mutex());
  *get_active_window_count() -= 1;
  if (*get_active_window_count() == 0) {
    glfwTerminate();
  }
  pthread_mutex_unlock(get_mutex());
}
