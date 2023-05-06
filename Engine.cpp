#include "Engine.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp> 
// Must come after vulkan include
#include <GLFW/glfw3.h>
#include <vulkan/vulkan_core.h>

namespace Sol {

#define WIDTH 800
#define HEIGHT 600

  /* Public members */
void Engine::init() {
  init_window(); 
  init_instance();
#if V_LAYERS 
  init_debug_messenger();
#endif
  init_surface();
  init_devices();
}

void Engine::kill() { 

#if V_LAYERS 
  DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
#endif

  vkDestroySurfaceKHR(instance, surface, nullptr);
  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();
}

GLFWwindow* Engine::get_window() {
  return window;
}
  /* Private members */
void Engine::init_window() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
  window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
  
  glfwSetKeyCallback(window, key_callback);
}

void Engine::init_instance() {
  VkApplicationInfo app_info{};
  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = "VK Engine";
  app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.pEngineName = "VK Engine";
  app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  app_info.apiVersion = VK_API_VERSION_1_3;

  VkInstanceCreateInfo instance_info;
  instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instance_info.pApplicationInfo = &app_info;
  instance_info.flags = 0x0;

  uint32_t glfw_ext_count;
  const char** glfw_ext = glfwGetRequiredInstanceExtensions(&glfw_ext_count);

#if V_LAYERS
  std::cout << "Initializing in debug mode...\n";
  const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
  Vec<const char*> exts;
  exts.init(glfw_ext_count + 1, allocator);
  mem_cpy(exts.data, glfw_ext, 8 * glfw_ext_count);
  exts.length = glfw_ext_count;
  exts.push(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  instance_info.ppEnabledLayerNames = layers;
  instance_info.enabledLayerCount = sizeof(layers) / 8;
  instance_info.enabledExtensionCount = exts.len();
  instance_info.ppEnabledExtensionNames = exts.data;
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  populate_debug_create_info(&debug_create_info);
  instance_info.pNext = 
    reinterpret_cast<VkDebugUtilsMessengerCreateInfoEXT*>(&debug_create_info);
#else 
  instance_info.enabledLayerCount = 0;
  instance_info.ppEnabledExtensionNames = glfw_ext;
  instance_info.enabledExtensionCount = glfw_ext_count;
  instance_info.pNext = nullptr;
#endif

  VkResult check = vkCreateInstance(&instance_info, nullptr, &instance);
  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT INSTANCE\n";
    abort();
  }
  std::cout << "VkInstance Initialized...\n";

#if V_LAYERS
  exts.kill();
#endif
}

void Engine::init_surface() {
  VkResult check = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT SURFACE\n";
    abort();
  }
  std::cout << "Initialized window surface...\n";
}

void Engine::init_devices() {
  const char* render_exts[] = {
    "VK_KHR_maintenance2",
    "VK_KHR_dynamic_rendering",
    "VK_KHR_swapchain",
  };
  const char* compute_exts[] = { };
  bool compute = true;
  uint32_t compute_list_size = sizeof(compute_exts) / sizeof(const char*);
  uint32_t render_list_size = sizeof(render_exts) / sizeof(const char*);
  PickDeviceResult compute_device_phys = 
    device_setup(compute, compute_exts, compute_list_size);
  PickDeviceResult render_device_phys = 
    device_setup(!compute, render_exts, render_list_size);

  VkDeviceCreateInfo create_compute_info;
  VkDeviceCreateInfo create_render_info;

  VkDeviceQueueCreateInfo render_queue_info;
  render_queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  render_queue_info.flags = 0x0;


  create_render_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_render_info.pNext = nullptr;
  create_render_info.flags = 0x0;
  create_render_info.queueCreateInfoCount = 1;

}

  /* Utility */
PickDeviceResult Engine::device_setup(bool compute, const char** ext_list, uint32_t list_size) { 
  RankDevicesResult ranking_result = rank_devices(compute);
  PickDeviceResult result;
  if (!compute) {
    for(uint32_t i = 0; i < ranking_result.queue_count; ++i) {
      VkBool32 check;
      vkGetPhysicalDeviceSurfaceSupportKHR(ranking_result.first_device, i, surface, &check);
      if (check) {
        result.present_queue_index = i;
        result.device = ranking_result.first_device;
        break;
      }
    }
    for(uint32_t i = 0; i < ranking_result.queue_count; ++i) {
      VkBool32 check;
      vkGetPhysicalDeviceSurfaceSupportKHR(ranking_result.second_device, i, surface, &check);
      if (check) {
        result.present_queue_index = i;
        result.device = ranking_result.second_device;
        break;
      }
    }
  } 
}

RankDevicesResult Engine::rank_devices(bool compute) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);

  VkPhysicalDevice physical_devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, physical_devices);

  VkPhysicalDevice primary[device_count];
  VkPhysicalDevice secondary[device_count];

  for(uint32_t i = 0; i < device_count; ++i) {
    VkPhysicalDeviceProperties device_props;
    vkGetPhysicalDeviceProperties(physical_devices[i], &device_props);
    uint32_t queue_prop_count;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_prop_count, nullptr);
    VkQueueFamilyProperties queue_props[queue_prop_count];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_devices[i], &queue_prop_count, queue_props);

    bool compute_support = false;
    bool graphics_support = false;

    for(uint32_t j = 0; j < queue_prop_count; ++j) {
      if (queue_props[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        graphics_support = true;
      }
      if (queue_props[j].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        compute_support = true;
      }
    }

    if (!compute) {
      if (
      device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && 
      graphics_support
      )
        primary[i] = physical_devices[i];
      else if (graphics_support)
        secondary[i] = physical_devices[i];
    } else {
      if (
      device_props.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && 
      compute_support
      )
        primary[i] = physical_devices[i];
      else if (compute_support) 
        secondary[i] = physical_devices[i];
    }
    
  }

  uint32_t max = 0;
  VkPhysicalDevice best_choice = physical_devices[0];
  VkPhysicalDevice second_choice = physical_devices[0];

  for(uint32_t i = 0; i < device_count; ++i) {
    uint32_t count = 0;
    for(uint32_t j = 0; j < device_count; ++j) {
      if (physical_devices[i] == primary[j]) 
        count += 1;
    }
    if (count > max) {
      max = count;
      second_choice = best_choice;
      best_choice = primary[i];
    }
    else if (count == max) {
      uint32_t max_best = 0;
      uint32_t max_curr = 0;
      for(uint32_t k = 0; k < device_count; ++k) {
        if (best_choice == secondary[k]) {
          max_best += 1;
        } else if (primary[i] == secondary[k]) {
          max_curr += 1;
        }
      }
      if (max_curr > max_best) {
        second_choice = best_choice;
        best_choice = primary[i];
      }
    }
  }

  uint32_t queue_count;
  vkGetPhysicalDeviceQueueFamilyProperties(best_choice, &queue_count, nullptr);
  RankDevicesResult result { 
    .first_device = best_choice,
    .second_device = second_choice,
    .queue_count = queue_count 
  };
  return result;
}

bool Engine::check_device_ext(VkPhysicalDevice dvc, VkDeviceCreateInfo *create_info) {
  return true;
}

bool Engine::check_device_layers(VkPhysicalDevice dvc, VkDeviceCreateInfo *create_info) {
  return true;
}

  /* Debug */
#if V_LAYERS
void Engine::init_debug_messenger() {
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  populate_debug_create_info(&debug_create_info);
  VkResult check = CreateDebugUtilsMessengerEXT(instance, &debug_create_info, nullptr, &debug_messenger);

  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT DEBUG MESSENGER\n";
    abort();
  }
}

void Engine::populate_debug_create_info(VkDebugUtilsMessengerCreateInfoEXT* create_info) {
  create_info->sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_info->flags = 0x0;
  create_info->messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_info->messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_info->pfnUserCallback = debug_callback;
  create_info->pNext = nullptr;
  create_info->pUserData = nullptr;
}

VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT messageType,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void* pUserData) {

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
  }

  return VK_FALSE;
}

VkResult Engine::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void Engine::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}
#endif
  /* END DEBUG */

  /* KeyBoard input */
void Engine::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

} // Sol
