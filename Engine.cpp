#include "Engine.hpp"

#include <iostream>
#include <vulkan/vulkan.hpp> 
#include <vulkan/vulkan_core.h>

// Must come after vulkan include
#include <GLFW/glfw3.h>

#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include "include/vk_mem_alloc.h"

namespace Sol {

#define WIDTH 800
#define HEIGHT 600

  /* Public members */
void Engine::run(Allocator* allocator) {
  init(allocator);

  handle_input();
}

void Engine::kill() { 

  vkDestroySwapchainKHR(devices.graphics, swapchain, nullptr);

  vmaDestroyAllocator(allocators.graphics);
  vmaDestroyAllocator(allocators.compute);

  vkDestroyDevice(devices.graphics, nullptr);
  vkDestroyDevice(devices.compute, nullptr);

  vkDestroySurfaceKHR(instance, surface, nullptr);

#if V_LAYERS 
  DestroyDebugUtilsMessengerEXT(instance, debug_messenger, nullptr);
#endif

  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);
  glfwTerminate();
}

  /* Private members */

// Initialization
void Engine::init(Allocator* allocator) {

  allocators.cpu = allocator;

  init_window(); 
  init_instance();
#if V_LAYERS 
  init_debug_messenger();
#endif
  init_surface();
  init_devices();
  init_allocators();
  init_swapchain();
}

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
  std::cout << "\n*** DEBUG MODE ON ***\n\n";
  const char* layers[] = { "VK_LAYER_KHRONOS_validation" };
  Vec<const char*> exts;
  exts.init(glfw_ext_count + 1, allocators.cpu);
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
  std::cout << "\n*** RELEASE MODE ***\n\n";
  instance_info.enabledLayerCount = 0;
  instance_info.ppEnabledExtensionNames = glfw_ext;
  instance_info.enabledExtensionCount = glfw_ext_count;
  instance_info.pNext = nullptr;
#endif

  VkResult check = vkCreateInstance(&instance_info, nullptr, &instance);
  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT INSTANCE (Aborting)...\n";
    abort();
  }
  std::cout << "Initialized VkInstance...\n";

#if V_LAYERS
  exts.kill();
#endif
}

void Engine::init_surface() {
  VkResult check = glfwCreateWindowSurface(instance, window, nullptr, &surface);
  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT SURFACE (Aborting)...\n";
    abort();
  }
  std::cout << "Initialized VkSurface...\n";
}

void Engine::init_devices() {

  const char* ext_list_graphics[] = RENDER_EXTENSIONS;
  const char* ext_list_compute[] = COMPUTE_EXTENSIONS;

  bool compute = true;
  uint32_t ext_list_size_graphics = sizeof(ext_list_graphics) / sizeof(const char*);
  uint32_t ext_list_size_compute = sizeof(ext_list_compute) / sizeof(const char*);

  DevicePickResult device_pick_result_graphics = 
    device_setup(!compute, ext_list_graphics, ext_list_size_graphics);
  DevicePickResult device_pick_result_compute = 
    device_setup(compute, ext_list_compute, ext_list_size_compute);

  const float queue_priorities[] = { 1.0f };

  uint32_t queue_indices_graphics[] = {
    device_pick_result_graphics.graphics_queue_index,
    device_pick_result_graphics.present_queue_index,
  };
  uint32_t queue_count_graphics = 2;
  VkDeviceQueueCreateInfo queue_infos_graphics[queue_count_graphics];

  for(uint32_t i = 0; i < queue_count_graphics; ++i) {
    queue_infos_graphics[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_infos_graphics[i].pNext = nullptr;
    queue_infos_graphics[i].flags = 0x0;
    queue_infos_graphics[i].queueFamilyIndex = queue_indices_graphics[i];
    queue_infos_graphics[i].queueCount = 1;
    queue_infos_graphics[i].pQueuePriorities = queue_priorities;
  }

  VkDeviceCreateInfo create_info_graphics;
  create_info_graphics.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info_graphics.pNext = nullptr;
  create_info_graphics.flags = 0x0;
  create_info_graphics.queueCreateInfoCount = queue_count_graphics;
  create_info_graphics.pQueueCreateInfos = queue_infos_graphics;
  create_info_graphics.enabledLayerCount = 0;
  create_info_graphics.ppEnabledLayerNames = nullptr;
  create_info_graphics.enabledExtensionCount = ext_list_size_graphics;
  create_info_graphics.ppEnabledExtensionNames = ext_list_graphics;
  // NOTE: I am interested to see when I will have to come back to this...
  create_info_graphics.pEnabledFeatures = nullptr;

  VkDeviceQueueCreateInfo queue_infos_compute[1];
  queue_infos_compute[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_infos_compute[0].pNext = nullptr;
  queue_infos_compute[0].flags = 0x0;
  queue_infos_compute[0].queueFamilyIndex = device_pick_result_compute.compute_queue_index;
  queue_infos_compute[0].queueCount = 1;
  queue_infos_compute[0].pQueuePriorities = queue_priorities;

  VkDeviceCreateInfo create_info_compute;
  create_info_compute.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  create_info_compute.pNext = nullptr;
  create_info_compute.flags = 0x0;
  create_info_compute.queueCreateInfoCount = 1;
  create_info_compute.pQueueCreateInfos = queue_infos_compute;
  create_info_compute.enabledLayerCount = 0;
  create_info_compute.ppEnabledLayerNames = nullptr;
  create_info_compute.enabledExtensionCount = ext_list_size_compute;
  create_info_compute.ppEnabledExtensionNames = ext_list_compute;
  // NOTE: I am interested to see when I will have to come back to this...
  create_info_compute.pEnabledFeatures = nullptr;

  VkResult creation_check_graphics = vkCreateDevice(device_pick_result_graphics.device, &create_info_graphics, nullptr, &devices.graphics);
  VkResult creation_check_compute = vkCreateDevice(device_pick_result_compute.device, &create_info_compute, nullptr, &devices.compute);

  bool creation_check_top = true;
  
  if (creation_check_graphics != VK_SUCCESS) {
    std::cout << "Failed to create render device!\n";
    creation_check_top = false;
  }
  if (creation_check_compute != VK_SUCCESS) {
    std::cout << "Failed to create compute device!\n";
    creation_check_top = false;
  }

  if (!creation_check_top) {
    std::cerr << "FAILED TO INIT LOGICAL DEVICES (Aborting)...\n";
  }

  devices.graphics_physical = device_pick_result_graphics.device;
  devices.graphics_queue = device_pick_result_graphics.graphics_queue_index;
  devices.present_queue = device_pick_result_graphics.present_queue_index;

  devices.compute_physical = device_pick_result_compute.device;
  devices.compute_queue = device_pick_result_compute.compute_queue_index;

  std::cout << "Initialized logical devices...\n";
}

void Engine::init_allocators() {
  
  VmaVulkanFunctions vulkan_functions = {};
  vulkan_functions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
  vulkan_functions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;
 
  VmaAllocatorCreateInfo gpu_allocator_info_graphics = {};
  gpu_allocator_info_graphics.vulkanApiVersion = VK_API_VERSION_1_3;
  gpu_allocator_info_graphics.physicalDevice = devices.graphics_physical;
  gpu_allocator_info_graphics.device = devices.graphics;
  gpu_allocator_info_graphics.instance = instance;
  gpu_allocator_info_graphics.pVulkanFunctions = &vulkan_functions;

  VmaAllocatorCreateInfo gpu_allocator_info_compute = {};
  gpu_allocator_info_compute.vulkanApiVersion = VK_API_VERSION_1_3;
  gpu_allocator_info_compute.physicalDevice = devices.compute_physical;
  gpu_allocator_info_compute.device = devices.compute;
  gpu_allocator_info_compute.instance = instance;
  gpu_allocator_info_compute.pVulkanFunctions = &vulkan_functions;
 
  VmaAllocator gpu_allocator_graphics;
  VkResult creation_check_alloc_graphics = vmaCreateAllocator(&gpu_allocator_info_graphics, &gpu_allocator_graphics);
  VmaAllocator gpu_allocator_compute;
  VkResult creation_check_alloc_compute = vmaCreateAllocator(&gpu_allocator_info_graphics, &gpu_allocator_compute);

  bool creation_check_alloc_top = true;
  if (creation_check_alloc_graphics != VK_SUCCESS) {
    std::cerr << "Failed to create compute allocator!\n";
    creation_check_alloc_top = false;
  }
  if (creation_check_alloc_compute != VK_SUCCESS) {
    std::cerr << "Failed to create compute allocator!\n";
    creation_check_alloc_top = false;
  }

  if (!creation_check_alloc_top) {
    std::cerr << "FAILED TO INITIALIZE GPU ALLOCATORS! (Aborting)...\n";
    abort();
  }

  allocators.graphics = gpu_allocator_graphics;
  allocators.compute = gpu_allocator_compute;
  std::cout << "Initialized GPU allocators...\n";
}

void Engine::init_swapchain() {
  PresentModes present_modes;
  get_present_mode_support(&present_modes);

  SurfaceFormatResult surface_format_result;
  get_format_color_space_support(&surface_format_result);

  VkSurfaceCapabilitiesKHR surface_capabilities;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(devices.graphics_physical, surface, &surface_capabilities);

  VkSwapchainCreateInfoKHR create_info_swapchain;
  create_info_swapchain.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  create_info_swapchain.pNext = nullptr;
  create_info_swapchain.flags = 0x0;
  create_info_swapchain.surface = surface;
  create_info_swapchain.minImageCount = 2;
  create_info_swapchain.imageExtent = surface_capabilities.currentExtent;
  create_info_swapchain.preTransform = surface_capabilities.currentTransform;
  create_info_swapchain.imageArrayLayers = 1;
  create_info_swapchain.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  create_info_swapchain.queueFamilyIndexCount = 0;
  create_info_swapchain.pQueueFamilyIndices = nullptr;
  create_info_swapchain.clipped = VK_TRUE;
  create_info_swapchain.oldSwapchain = VK_NULL_HANDLE;

  if (present_modes.mailbox)
    create_info_swapchain.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
  else 
    create_info_swapchain.presentMode = VK_PRESENT_MODE_FIFO_KHR;
  if (surface_format_result.tuple_1) {
    create_info_swapchain.imageFormat = VK_FORMAT_B8G8R8A8_SRGB;
    create_info_swapchain.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
  } else {
    create_info_swapchain.imageFormat = surface_format_result.first_specified.format;
    create_info_swapchain.imageColorSpace = surface_format_result.first_specified.colorSpace;
  }

  bool top_check_swapchain_creation = true;
  uint32_t err_msgs_size = 3;
  const char* err_msgs[err_msgs_size]; 
  for(uint32_t i = 0; i < err_msgs_size; ++i) {
    err_msgs[i] = "\0";
  }
  
  if (surface_capabilities.minImageCount >= 2) 
    create_info_swapchain.minImageCount = 2;
  else {
    top_check_swapchain_creation = false;
    err_msgs[0] = "Insufficient minImageCount (count < 2)\n";
  }
  if (surface_capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
    create_info_swapchain.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  else {
    top_check_swapchain_creation = false;
    err_msgs[1] = "Missing Alpha Opaque Bit (CompositeAlphaFlags)\n";
  }
  if (surface_capabilities.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) 
    create_info_swapchain.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; 
  else {
    top_check_swapchain_creation = false;
    err_msgs[2] = "Missing Color Attachment bit (ImageUsage)\n";
  }

  if (!top_check_swapchain_creation) {
    std::cerr << "ERRORS IN SWAPCHAIN CREATION:\n";
    for(uint32_t i = 0; i < err_msgs_size; ++i) {
      if (strcmp(err_msgs[i], "\0") != 0) 
        std::cerr << "  " << err_msgs[i];
    }
    std::cerr << "(Aborting...)\n";
    abort();
  }

  VkResult creation_check_swapchain = 
    vkCreateSwapchainKHR(devices.graphics, &create_info_swapchain, nullptr, &swapchain);
  if (creation_check_swapchain != VK_SUCCESS) {
    std::cerr << "ERROR INITIALIZING SWAPCHAIN (Aborting...)\n";
    abort();
  }

  std::cout << "Initialized Swapchain...\n";
}

  /* End Initializations */

  /* Running */

void Engine::handle_input() {
  while(!glfwWindowShouldClose(window)) 
    glfwPollEvents();
}

void Engine::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
  if (key == GLFW_KEY_Q && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, true);
}

  /* Utility */
DevicePickResult Engine::device_setup(bool compute, const char** ext_list, uint32_t list_size) { 
  DevicePickResult result;

  if (!compute) {
    result = render_device_setup(ext_list, list_size);
  }
  else 
    result = compute_device_setup(ext_list, list_size);

  return result;
}

DevicePickResult Engine::render_device_setup(const char** ext_list, uint32_t list_size) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  VkPhysicalDevice devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  DevicePickResult secondary;
  
  for(uint32_t device_index = 0; device_index < device_count; ++device_index) {
    DevicePickResult prelim_result;
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

DevicePickResult Engine::compute_device_setup(const char** ext_list, uint32_t list_size) {
  uint32_t device_count;
  vkEnumeratePhysicalDevices(instance ,&device_count, nullptr);
  VkPhysicalDevice devices[device_count];
  vkEnumeratePhysicalDevices(instance, &device_count, devices);

  DevicePickResult secondary;

  for(uint32_t device_index = 0; device_index < device_count; ++device_index) {

    DevicePickResult prelim_result;
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

void Engine::get_present_mode_support(PresentModes *present_modes) {
  uint32_t present_mode_count;
  vkGetPhysicalDeviceSurfacePresentModesKHR(devices.graphics_physical, surface, &present_mode_count, nullptr);
  VkPresentModeKHR modes[present_mode_count];  
  vkGetPhysicalDeviceSurfacePresentModesKHR(devices.graphics_physical, surface, &present_mode_count, nullptr);

  for(uint32_t i = 0; i < present_mode_count; ++i) {
    if (modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR) 
      present_modes->immediate = true;
    if (modes[i] == VK_PRESENT_MODE_MAILBOX_KHR) 
      present_modes->mailbox = true;
    if (modes[i] == VK_PRESENT_MODE_FIFO_KHR) 
      present_modes->fifo = true;
  }

}

void Engine::get_format_color_space_support(SurfaceFormatResult *surface_format_result) {
  uint32_t format_count;
  vkGetPhysicalDeviceSurfaceFormatsKHR(devices.graphics_physical, surface, &format_count, nullptr);
  VkSurfaceFormatKHR formats[format_count];
  vkGetPhysicalDeviceSurfaceFormatsKHR(devices.graphics_physical, surface, &format_count, formats);

  for(uint32_t format_index = 0; format_index < format_count; ++format_index) {
    if (
      formats[format_index].format == VK_FORMAT_B8G8R8A8_SRGB &&
      formats[format_index].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR
    ) {
      surface_format_result->tuple_1 = true;
      break;
    }
  }
  surface_format_result->tuple_1 = false;
  surface_format_result->first_specified = formats[0];
}

  /* Utility End */

  /* Debug */
#if V_LAYERS
void Engine::init_debug_messenger() {
  VkDebugUtilsMessengerCreateInfoEXT debug_create_info;
  populate_debug_create_info(&debug_create_info);
  VkResult check = CreateDebugUtilsMessengerEXT(instance, &debug_create_info, nullptr, &debug_messenger);

  if (check != VK_SUCCESS) {
    std::cerr << "FAILED TO INIT DEBUG MESSENGER (Aborting)...\n";
    abort();
  }
  std::cout << "Initialized Debug Messenger...\n";
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

} // Sol
