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

#define RENDER_EXTENSIONS { "VK_KHR_maintenance2", "VK_KHR_dynamic_rendering", "VK_KHR_swapchain", }
#define COMPUTE_EXTENSIONS { }

struct PickDeviceResult {
  VkPhysicalDevice device;
  uint32_t present_queue_index;
  uint32_t compute_queue_index;
  uint32_t graphics_queue_index;
  // Bool in struct is irrelevant here.
  bool present_found = false;
  bool compute_found = false;
  bool graphics_found = false;
  bool extensions_support = false;
};

struct Engine {
public:
  Allocator* cpu_allocator;
  void init();
  void kill();
  GLFWwindow* get_window();

private:
  GLFWwindow* window;
  VkSurfaceKHR surface;
  VkInstance instance;
  VkDevice render_device;
  VkDevice compute_device;
  // Initializations
  void init_window();
  void init_instance();
  void init_devices();
  void init_surface();
  // Utility
  PickDeviceResult device_setup(bool compute, const char** ext_list, uint32_t list_size);
  PickDeviceResult compute_device_setup(const char** ext_list, uint32_t list_size);
  PickDeviceResult render_device_setup(const char** ext_list, uint32_t list_size);

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
