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

  vkDestroyDevice(render_device, nullptr);
  vkDestroyDevice(compute_device, nullptr);
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
  exts.init(glfw_ext_count + 1, cpu_allocator);
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
}

void Engine::init_devices() {

  const char* render_extensions[] = RENDER_EXTENSIONS;
  const char* compute_extensions[] = COMPUTE_EXTENSIONS;

  bool compute = true;
  uint32_t render_list_size = sizeof(render_extensions) / sizeof(const char*);
  uint32_t compute_list_size = sizeof(compute_extensions) / sizeof(const char*);

  PickDeviceResult render_device_phys = 
    device_setup(!compute, render_extensions, render_list_size);
  PickDeviceResult compute_device_phys = 
    device_setup(compute, compute_extensions, compute_list_size);

  const float queue_priorities[] = { 1.0f };

  uint32_t render_queue_indices[] = {
    render_device_phys.graphics_queue_index,
    render_device_phys.present_queue_index,
  };
  uint32_t render_device_queue_count = 2;
  VkDeviceQueueCreateInfo render_queue_infos[render_device_queue_count];

  for(uint32_t i = 0; i < render_device_queue_count; ++i) {
    render_queue_infos[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    render_queue_infos[i].pNext = nullptr;
    render_queue_infos[i].flags = 0x0;
    render_queue_infos[i].queueFamilyIndex = render_queue_indices[i];
    render_queue_infos[i].queueCount = 1;
    render_queue_infos[i].pQueuePriorities = queue_priorities;
  }

  VkDeviceCreateInfo render_create_info;
  render_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  render_create_info.pNext = nullptr;
  render_create_info.flags = 0x0;
  render_create_info.queueCreateInfoCount = render_device_queue_count;
  render_create_info.pQueueCreateInfos = render_queue_infos;
  render_create_info.enabledLayerCount = 0;
  render_create_info.ppEnabledLayerNames = nullptr;
  render_create_info.enabledExtensionCount = render_list_size;
  render_create_info.ppEnabledExtensionNames = render_extensions;
  // NOTE: I am interested to see when I will have to come back to this...
  render_create_info.pEnabledFeatures = nullptr;

  VkDeviceQueueCreateInfo compute_queue_infos[1];
  compute_queue_infos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  compute_queue_infos[0].pNext = nullptr;
  compute_queue_infos[0].flags = 0x0;
  compute_queue_infos[0].queueFamilyIndex = compute_device_phys.compute_queue_index;
  compute_queue_infos[0].queueCount = 1;
  compute_queue_infos[0].pQueuePriorities = queue_priorities;

  VkDeviceCreateInfo compute_create_info;
  compute_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  compute_create_info.pNext = nullptr;
  compute_create_info.flags = 0x0;
  compute_create_info.queueCreateInfoCount = 1;
  compute_create_info.pQueueCreateInfos = compute_queue_infos;
  compute_create_info.enabledLayerCount = 0;
  compute_create_info.ppEnabledLayerNames = nullptr;
  compute_create_info.enabledExtensionCount = compute_list_size;
  compute_create_info.ppEnabledExtensionNames = compute_extensions;
  // NOTE: I am interested to see when I will have to come back to this...
  compute_create_info.pEnabledFeatures = nullptr;

  VkResult render_creation_check = vkCreateDevice(render_device_phys.device, &render_create_info, nullptr, &render_device);
  // TODO: Something in the compute creation check is segfaulting...
  // There will some stupid unitialized pointer in the compute setup func...
  VkResult compute_creation_check = vkCreateDevice(compute_device_phys.device, &compute_create_info, nullptr, &compute_device);

  if (render_creation_check != VK_SUCCESS) {
    std::cout << "FAILED TO CREATE RENDER DEVICE!\n";
    abort();
  }
  if (compute_creation_check != VK_SUCCESS) {
    std::cout << "FAILED TO CREATE COMPUTE DEVICE!\n";
    abort();
  }

  std::cout << "Initialized logical devices...\n";
}

  /* Utility */
PickDeviceResult Engine::device_setup(bool compute, const char** ext_list, uint32_t list_size) { 
  PickDeviceResult result;

  if (!compute) {
    result = render_device_setup(ext_list, list_size);
  }
  else 
    result = compute_device_setup(ext_list, list_size);

  return result;
}

PickDeviceResult Engine::render_device_setup(const char** ext_list, uint32_t list_size) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  VkPhysicalDevice devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  PickDeviceResult secondary;
  
  for(uint32_t device_index = 0; device_index < device_count; ++device_index) {
    PickDeviceResult prelim_result;
    prelim_result.device = devices[device_index];
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_count, nullptr);
    VkQueueFamilyProperties queue_props[queue_count];
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_count, queue_props);
    for(uint32_t queue_index = 0; queue_index < queue_count; ++queue_index) {
      VkBool32 pres_check;
      vkGetPhysicalDeviceSurfaceSupportKHR(devices[device_index], queue_index, surface, &pres_check);
      if (pres_check) {
        prelim_result.present_found = true;
        prelim_result.present_queue_index = queue_index;
      }
      if (queue_props[queue_index].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
        prelim_result.graphics_found = true;
        prelim_result.graphics_queue_index = queue_index;
      }
    }
    uint32_t extension_count;
    vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extension_count, nullptr);
    VkExtensionProperties ext_props[extension_count];
    vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extension_count, ext_props);
    uint32_t check_ext_count = 0;
    for(uint32_t ext_list_index = 0; ext_list_index < list_size; ++ext_list_index) {
      for(uint32_t extension_index = 0; extension_index < extension_count; ++extension_index) {
        if (strcmp(ext_list[ext_list_index], ext_props[extension_index].extensionName) == 0) {
          check_ext_count += 1;
        }
      }
      if (check_ext_count == list_size) {
        prelim_result.extensions_support = true;
        break;
      }
    }

    if 
    (
      prelim_result.present_found && 
      prelim_result.graphics_found && 
      prelim_result.extensions_support || 
      list_size == 0
    ) 
    {
      VkPhysicalDeviceProperties device_props;
      vkGetPhysicalDeviceProperties(devices[device_index], &device_props);
      if (device_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        // This returns the primary choice (all boxes ticked)
        return prelim_result;
      }
      else
        secondary = prelim_result;
    }
  }

  // Primary choice has already been found and returned, so just return secondary
  return secondary;
}

PickDeviceResult Engine::compute_device_setup(const char** ext_list, uint32_t list_size) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance ,&device_count, nullptr);
  VkPhysicalDevice devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  PickDeviceResult secondary;

  for(uint32_t device_index = 0; device_index < device_count; ++device_index) {

    PickDeviceResult prelim_result;
    prelim_result.device = devices[device_index];
    
    uint32_t queue_count;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_count, nullptr);
    VkQueueFamilyProperties queues[queue_count];
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_count, queues);
    for(uint32_t queue_index = 0; queue_index < queue_count; ++queue_index) {
      if (queues[queue_index].queueFlags & VK_QUEUE_COMPUTE_BIT) {
        prelim_result.compute_found = true;
        prelim_result.compute_queue_index = queue_index;
      }
    }

    for(uint32_t ext_list_index = 0; ext_list_index < list_size; ++ext_list_index) {
      uint32_t extension_count;
      vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extension_count, nullptr);
      VkExtensionProperties extension_props[extension_count];
      vkEnumerateDeviceExtensionProperties(devices[device_index], nullptr, &extension_count, extension_props);
      uint32_t ext_check_count = 0;
      for(uint32_t extension_index = 0; extension_index < extension_count; ++extension_index) {
        if (strcmp(ext_list[ext_list_index], extension_props[extension_index].extensionName) == 0) {
          ext_check_count += 1;
        }
      }

      if (ext_check_count == list_size) {
        prelim_result.extensions_support = true;
        break;
      }
    }

    if 
    (
      prelim_result.compute_found && 
      prelim_result.extensions_support || 
      list_size == 0
    ) 
    {
      VkPhysicalDeviceProperties device_props;
      vkGetPhysicalDeviceProperties(devices[device_index], &device_props);
      if (device_props.deviceType & VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) 
        return prelim_result;
      else 
        secondary = prelim_result;
    }
  }
  return secondary;
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
