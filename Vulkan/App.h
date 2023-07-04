#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>
#include <fstream>

const uint32_t WIN_WIDTH = 800;
const uint32_t WIN_HEIGHT = 600;
const int MAX_FRAMES_IN_FLIGHT = 2;

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
    VkSurfaceCapabilitiesKHR capabilities{};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class App
{
public:
    void run();

private:
    static std::vector<char> readFile( const std::string &filename );
    
    void setupDebugMessenger();

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback( 
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
        void *pUserData );

     VkResult CreateDebugUtilsMessengerEXT( 
         VkInstance m_Instance,
         const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
         const VkAllocationCallbacks *pAllocator,
         VkDebugUtilsMessengerEXT *pDebugMessenger );

     void DestroyDebugUtilsMessengerEXT( 
         VkInstance m_Instance,
         VkDebugUtilsMessengerEXT debugMessenger,
         const VkAllocationCallbacks *pAllocator );

     void populateDebugMessengerCreateInfo( 
         VkDebugUtilsMessengerCreateInfoEXT &createInfo );
    
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
    void createLogicalDevice();
    void createSwapChain();
    void createImageView();
    void createRenderPass();
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSwapChain();

    void cleanupSwapchain();

    VkShaderModule createShaderModule( const std::vector<char> &code );
    SwapChainSupportDetails querySwapChainSupport( VkPhysicalDevice device );
    VkSurfaceFormatKHR chooseSwapSurfaceFormat( const std::vector<VkSurfaceFormatKHR> &availableFormats ) const;
    VkPresentModeKHR chooseSwapPresentMode( const std::vector<VkPresentModeKHR> &availablePresentModes ) const;
    VkExtent2D chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities ) const;
    
    void recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex );

    void pickphysicalDevice();
    bool isDeviceSuitable( VkPhysicalDevice m_Device );
    QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );
    bool checkDeviceExtensionSupport( VkPhysicalDevice device );

private: // Window Application
    GLFWwindow *m_Window = nullptr;

private: // Vulkan API
    VkInstance m_Instance;
    VkDebugUtilsMessengerEXT debugMessenger;

    VkDevice         m_Device;
    VkPhysicalDevice m_PhysicalDevice;
    VkQueue          m_GraphicsQueue;
    VkSurfaceKHR     m_Surface;
    VkQueue          m_PresentQueue;
    VkSwapchainKHR   m_SwapChain;
    VkCommandPool    m_CommandPool;

    uint32_t m_CurrentFrame = 0;
    std::vector<VkSemaphore>  m_ImageAvailableSemaphores;
    std::vector<VkSemaphore>  m_RenderFinishedSemaphores;
    std::vector<VkFence>      m_InFlightFences;

    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    std::vector<VkImage>       m_SwapChainImages;
    std::vector<VkImageView>   m_SwapChainImageViews;
    VkFormat                   m_SwapChainImageFormat;
    VkExtent2D                 m_SwapChainExtent;

    VkRenderPass     m_RenderPass;
    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_GraphicsPipeline;
};
