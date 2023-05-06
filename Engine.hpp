#pragma once 

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
// Must come after vulkan include 
#include <GLFW/glfw3.h>

#include "common/Allocator.hpp"
#include "common/Vec.hpp"

namespace Sol {

#define DEBUG

#if defined DEBUG
  #define V_LAYERS true
#else
  #define V_LAYERS false
#endif

struct PickDeviceResult {
  VkPhysicalDevice device;
  uint32_t present_queue_index;
  uint32_t compute_queue_index;
  uint32_t graphics_queue_index;
};

struct RankDevicesResult {
  VkPhysicalDevice first_device;
  VkPhysicalDevice second_device;
  uint32_t queue_count;
};

struct Engine {
public:
  Allocator* allocator;
  void init();
  void kill();
  GLFWwindow* get_window();

private:
  GLFWwindow* window;
  VkSurfaceKHR surface;
  VkInstance instance;
  VkDevice compute_device;
  VkDevice render_device;
  // Initializations
  void init_window();
  void init_instance();
  void init_devices();
  void init_surface();
  // Utility
  PickDeviceResult device_setup(bool compute, const char** ext_list, uint32_t list_size);
  RankDevicesResult rank_devices(bool compute);
  bool check_device_ext(VkPhysicalDevice device, VkDeviceCreateInfo *create_info);
  bool check_device_layers(VkPhysicalDevice device, VkDeviceCreateInfo *create_info);

  // Window input
  static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

  // Debug 
#if V_LAYERS 
  VkDebugUtilsMessengerEXT debug_messenger;

  void init_debug_messenger();
  static void populate_debug_create_info(VkDebugUtilsMessengerCreateInfoEXT* create_info);
  static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
  VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
  void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
#endif
};

} // Sol
