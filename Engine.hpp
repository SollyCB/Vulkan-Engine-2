#pragma once 

#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_core.h>
// Must come after vulkan include 
#include <GLFW/glfw3.h>

#include "common/Allocator.hpp"
#include "common/Vec.hpp"
#include "include/vk_mem_alloc.h"

namespace Sol {

#define DEBUG

#if defined DEBUG
  #define V_LAYERS true
#else
  #define V_LAYERS false
#endif

#define RENDER_EXTENSIONS { "VK_KHR_maintenance2", "VK_KHR_dynamic_rendering", "VK_KHR_swapchain", }
#define COMPUTE_EXTENSIONS { }

struct DevicePickResult {
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

struct Devices {
  VkDevice graphics;
  VkDevice compute;
  VkPhysicalDevice graphics_physical;
  VkPhysicalDevice compute_physical;
  uint8_t graphics_queue;
  uint8_t present_queue;
  uint8_t compute_queue;
};

struct EngineAllocators {
  Allocator* cpu;
  VmaAllocator graphics;
  VmaAllocator compute;
};

struct PresentModes {
  bool immediate = false;
  bool mailbox = false;
  bool fifo = false;
};

enum class SurfaceFormatTuple1 {
  VK_FORMAT_B8G8R8A8_SRGB,
  VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
};

struct SurfaceFormatResult {
  VkSurfaceFormatKHR first_specified;
  bool tuple_1 = false;
};

struct SwapchainSettings {
  VkPresentModeKHR present_mode;
  VkFormat image_format;
  VkColorSpaceKHR color_space;
};

struct Engine {
public:
  void run(Allocator* allocator);
  void kill();

private:

  GLFWwindow* window;
  EngineAllocators allocators;
  VkSurfaceKHR surface;
  VkInstance instance;
  Devices devices;
  VkSwapchainKHR swapchain;
  SwapchainSettings swapchain_settings;

  // Initializations
  void init(Allocator* allocator);
  void init_window();
  void init_instance();
  void init_surface();
  void init_devices();
  void init_allocators();
  void init_swapchain();

  // Running
  void handle_input();
  void recreate_swapchain();
  static void resize_callback(GLFWwindow* window, int width, int height);
  static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
  
  // Utility
    // Prefers integrated GPU for compute, and discrete for graphics. 
  DevicePickResult device_setup(bool compute, const char** ext_list, uint32_t list_size);
  DevicePickResult compute_device_setup(const char** ext_list, uint32_t list_size);
  DevicePickResult render_device_setup(const char** ext_list, uint32_t list_size);
  void get_present_mode_support(PresentModes *present_modes);
  void get_format_color_space_support(SurfaceFormatResult *surface_format_result);

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
