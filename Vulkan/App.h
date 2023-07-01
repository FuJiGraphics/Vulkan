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

  static std::vector<char> readFile( const std::string &filename );


private:
    void initWindow();
    void initVulkan();
    void mainLoop();
    void cleanup();

private:
    bool checkValidationLayerSupport() const;
    // TODO: 메시지 콜백 레이어 활성화

private:
    void createInstance();
    void createSurface();
    void createSwapChain();
    void createImageView();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();

    VkShaderModule createShaderModule( const std::vector<char> &code );
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats ) const;
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes ) const;
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities ) const;

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
    VkInstance instance = VK_NULL_HANDLE;

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain = VK_NULL_HANDLE;

    std::vector<VkFramebuffer> swapChainFramebuffers;

    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;

    VkPipeline graphicsPipeline;
};
