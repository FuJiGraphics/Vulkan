#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#include <GLFW/glfw3.h>

#include <optional>
#include <vector>
#include <fstream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <array>

#include <chrono>

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

struct UniformBufferObject
{
    alignas( 16 ) glm::mat4 model;
    alignas( 16 ) glm::mat4 view;
    alignas( 16 ) glm::mat4 proj;
};

struct Vertex
{
    glm::vec2 pos;
    glm::vec3 color;

    static VkVertexInputBindingDescription getBindingDescription()
    {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof( Vertex );
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription()
    {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof( Vertex, pos );

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof( Vertex, color );

        return attributeDescriptions;
    }
};

const std::vector<Vertex> triangle = { { {  0.0f,  -0.5f }, { 1.0f, 0.0f, 0.0f } },
                                       { {  0.5f,   0.5f }, { 0.0f, 1.0f, 0.0f } },
                                       { { -0.5f,   0.5f }, { 0.0f, 0.0f, 1.0f } } };

const std::vector<Vertex> rectangle = { { { -0.5f, -0.5f }, { 1.0f, 0.0f, 0.0f } },
                                        { {  0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f } },
                                        { {  0.5f,  0.5f }, { 0.0f, 0.0f, 1.0f } },
                                        { { -0.5f,  0.5f }, { 0.0f, 0.0f, 0.0f } } };

const std::vector<uint16_t> indices = { 0, 1, 2, 2, 3, 0 };

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

     static void framebufferResizeCallback( GLFWwindow *window, int width, int height );

     uint32_t findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties );
     
     void copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size );


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
    void createDescriptorSetLayout(); 
    void createGraphicsPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void createCommandBuffers();
    void createSyncObjects();
    void recreateSwapChain();

    void cleanupSwapchain();

    void updateUniformBuffer( uint32_t currentImage );

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

    void createBuffer( VkDeviceSize size,
                       VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags properties,
                       VkBuffer &buffer,
                       VkDeviceMemory &bufferMemory );

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

    bool m_FramebufferResized = false;

    VkBuffer m_VertexBuffer;
    VkDeviceMemory m_VertexBufferMemory;
    VkBuffer m_IndexBuffer;
    VkDeviceMemory m_IndexBufferMemory;

    std::vector<VkBuffer> m_UniformBuffers;
    std::vector<VkDeviceMemory> m_UniformBuffersMemory;
    std::vector<void *> m_UniformBuffersMapped;
    
    VkDescriptorPool m_DescriptorPool;
    std::vector<VkDescriptorSet> m_DescriptorSets;

    std::vector<VkCommandBuffer> m_CommandBuffers;

    std::vector<VkFramebuffer> m_SwapChainFramebuffers;
    std::vector<VkImage>       m_SwapChainImages;
    std::vector<VkImageView>   m_SwapChainImageViews;
    VkFormat                   m_SwapChainImageFormat;
    VkExtent2D                 m_SwapChainExtent;

    VkRenderPass     m_RenderPass;
    VkDescriptorSetLayout m_DescriptorSetLayout;
    VkPipelineLayout m_PipelineLayout;

    VkPipeline m_GraphicsPipeline;
};
