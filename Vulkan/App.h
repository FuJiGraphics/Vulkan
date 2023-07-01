#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>
#include <fstream>

const uint32_t gWinWidth = 800;
const uint32_t gWinHeight = 600;

const std::vector<const char *> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const std::vector<const char *> deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

struct QueueFamilyIndices
{
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() const
    {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class HelloTriangleApplication
{
public:
    void run();

private:
  static std::vector<char> readFile( const std::string &filename );
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                       VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                       const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                       void *pUserData );
  VkResult CreateDebugUtilsMessengerEXT( VkInstance instance,
                                         const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                         const VkAllocationCallbacks *pAllocator,
                                         VkDebugUtilsMessengerEXT *pDebugMessenger );
  void DestroyDebugUtilsMessengerEXT( VkInstance instance,
                                      VkDebugUtilsMessengerEXT debugMessenger,
                                      const VkAllocationCallbacks *pAllocator );
  void populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo );
  std::vector<const char *> getRequiredExtensions();

private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();
    
    void drawFrame();

private:
    bool checkValidationLayerSupport() const;

  private:
    void createInstance();
    void createSurface();
    void createSwapChain();
    void createImageView();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSemaphores();
    void setupDebugMessenger();

    VkShaderModule createShaderModule( const std::vector<char> &code );
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats ) const;
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes ) const;
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities ) const;
    
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex );

  private: // Physical Devices and Queue Families
    void pickphysicalDevice();
    bool isDeviceSuitable( VkPhysicalDevice device );
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );
    bool checkDeviceExtensionSupport( VkPhysicalDevice device );

private: // Logical Device and queues
    void createLogicalDevice();

private: // Window Application
    GLFWwindow *window = nullptr;

private: // Vulkan API
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkPhysicalDevice    physicalDevice;
    VkDevice            device;
    VkQueue             graphicsQueue;
    VkSurfaceKHR        surface;
    VkQueue             presentQueue;
    VkSwapchainKHR      swapChain;
    VkCommandPool       commandPool;

    std::vector<VkFramebuffer> swapChainFramebuffers;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;
};
