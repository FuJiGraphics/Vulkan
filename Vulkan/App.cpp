#include "App.h"

#include <cstdlib>
#include <iostream>
#include <optional>
#include <set>
#include <stdexcept>

void App::run()
{
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

std::vector<char> App::readFile( const std::string &filename )
{
    std::ifstream file( filename, std::ios::ate | std::ios::binary );
    if ( !file.is_open() )
    {
        throw std::runtime_error( "failed to open file!" );
    }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer( fileSize );

    file.seekg( 0 );
    file.read( buffer.data(), fileSize );

    file.close();

    return buffer;
}

VKAPI_ATTR VkBool32 VKAPI_CALL
App::debugCallback( VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                         VkDebugUtilsMessageTypeFlagsEXT messageType,
                                         const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                         void *pUserData )
{

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;
    return VK_FALSE;
}

VkResult App::CreateDebugUtilsMessengerEXT( 
    VkInstance m_Instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pDebugMessenger )
{
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr( m_Instance, "vkCreateDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        return func( m_Instance, pCreateInfo, pAllocator, pDebugMessenger );
    }
    else
    {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void App::DestroyDebugUtilsMessengerEXT( VkInstance m_Instance,
                                                              VkDebugUtilsMessengerEXT debugMessenger,
                                                              const VkAllocationCallbacks *pAllocator )
{
    auto func =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr( m_Instance, "vkDestroyDebugUtilsMessengerEXT" );
    if ( func != nullptr )
    {
        func( m_Instance, debugMessenger, pAllocator );
    }
}

void App::populateDebugMessengerCreateInfo( VkDebugUtilsMessengerCreateInfoEXT &createInfo )
{
    createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
}

std::vector<const char *> App::getRequiredExtensions()
{
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions;
    glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

    std::vector<const char *> extensions( glfwExtensions, glfwExtensions + glfwExtensionCount );

    if ( enableValidationLayers )
    {
        extensions.push_back( VK_EXT_DEBUG_UTILS_EXTENSION_NAME );
    }

    return extensions;
}

void App::framebufferResizeCallback( GLFWwindow *window, int width, int height )
{
    auto app = reinterpret_cast<App *>( glfwGetWindowUserPointer( window ) );
    app->m_FramebufferResized = true;
}

uint32_t App::findMemoryType( uint32_t typeFilter, VkMemoryPropertyFlags properties )
{
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties( m_PhysicalDevice, &memProperties );
    for ( uint32_t i = 0; i < memProperties.memoryTypeCount; ++i )
    {
        /*
        VkMemoryRequirements::memoryTypeBits is a bitfield that sets a bit for every memoryType 
        that is supported for the resource. Therefore we need to check if the bit at index i is
        set while also testing the required memory property flags while iterating over the memory 
        types. Leaving this here just in case I'm not the only one that got confused.
        */
        if ( typeFilter & ( 1 << i ) && 
             ( memProperties.memoryTypes[i].propertyFlags & properties ) == properties )
        {
            return i;
        }
    }

    throw std::runtime_error( "Failed to find suitable memory type!" );

    return 0;
}

void App::copyBuffer( VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size )
{
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers( m_Device, &allocInfo, &commandBuffer );

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

   vkBeginCommandBuffer( commandBuffer, &beginInfo );

   VkBufferCopy copyRegion{};
   copyRegion.srcOffset = 0; // Optional
   copyRegion.dstOffset = 0; // Optional
   copyRegion.size = size; // can't use the VK_WOLE_SIZE
   vkCmdCopyBuffer( commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion );

   vkEndCommandBuffer( commandBuffer );

   VkSubmitInfo submitInfo{};
   submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
   submitInfo.commandBufferCount = 1;
   submitInfo.pCommandBuffers = &commandBuffer;
   vkQueueSubmit( m_GraphicsQueue, 1, &submitInfo, VK_NULL_HANDLE );
   vkQueueWaitIdle( m_GraphicsQueue );

   vkFreeCommandBuffers( m_Device, m_CommandPool, 1, &commandBuffer );
}

void App::recordCommandBuffer( VkCommandBuffer commandBuffer, uint32_t imageIndex )
{
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;                  // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    if ( vkBeginCommandBuffer( commandBuffer, &beginInfo ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to begin recording command buffer!" );
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_RenderPass;
    renderPassInfo.framebuffer = m_SwapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = { 0, 0 };
    renderPassInfo.renderArea.extent = m_SwapChainExtent;

    VkClearValue clearColor = { 0.0f, 0.0f, 0.0f, 1.0f };
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
    vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_GraphicsPipeline );
   
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>( m_SwapChainExtent.width );
    viewport.height = static_cast<float>( m_SwapChainExtent.height );
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport( commandBuffer, 0, 1, &viewport );
    
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = m_SwapChainExtent;
    vkCmdSetScissor( commandBuffer, 0, 1, &scissor );

    VkBuffer vertexBuffers[] = { m_VertexBuffer };
    VkDeviceSize offsets[] = { 0 };
    vkCmdBindVertexBuffers( commandBuffer, 0, 1, vertexBuffers, offsets );
    vkCmdBindIndexBuffer( commandBuffer, m_IndexBuffer, 0, VK_INDEX_TYPE_UINT16 );

    // vkCmdDraw( commandBuffer, static_cast<uint32_t>( triangle.size() ), 1, 0, 0 );
    
    vkCmdBindDescriptorSets( m_CommandBuffers[m_CurrentFrame], 
                             VK_PIPELINE_BIND_POINT_GRAPHICS, 
                             m_PipelineLayout, 0, 1,
                             m_DescriptorSets.data(), 0, nullptr );

    vkCmdDrawIndexed( commandBuffer, static_cast<uint32_t>( indices.size() ), 1, 0, 0, 0 );
    
    vkCmdEndRenderPass( commandBuffer );
    if ( vkEndCommandBuffer( commandBuffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to record command buffer!" );
    }
}


void App::initWindow()
{
    // These built-in functions are the first step to the necessary.
    glfwInit();
    // GLFW was originally desinged to create an OpenGL context.
    // we need to tell it to not create an OpenGL context with a subsequent call
    glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
    // to Disable the configuration a resize windows.
    glfwWindowHint( GLFW_RESIZABLE, GLFW_TRUE );

    m_Window = glfwCreateWindow( WIN_WIDTH, WIN_HEIGHT, "Draw Triangle", nullptr, nullptr );
    glfwSetWindowUserPointer( m_Window, this );
    glfwSetFramebufferSizeCallback( m_Window, framebufferResizeCallback );
}

void App::initVulkan()
{
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickphysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageView();
    createRenderPass();
    createDescriptorSetLayout();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createVertexBuffer();
    createIndexBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
    createSyncObjects();
}

void App::mainLoop()
{
    while ( !glfwWindowShouldClose( m_Window ) )
    {
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle( m_Device );
}

void App::cleanup()
{
    cleanupSwapchain();

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroyBuffer( m_Device, m_UniformBuffers[i], nullptr );
        vkFreeMemory( m_Device, m_UniformBuffersMemory[i], nullptr );
    }

    vkDestroyDescriptorPool( m_Device, m_DescriptorPool, nullptr );
    vkDestroyDescriptorSetLayout( m_Device, m_DescriptorSetLayout, nullptr );
   
    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        vkDestroySemaphore( m_Device, m_ImageAvailableSemaphores[i], nullptr );
        vkDestroySemaphore( m_Device, m_RenderFinishedSemaphores[i], nullptr );
        vkDestroyFence( m_Device, m_InFlightFences[i], nullptr );
    }
    vkDestroyBuffer( m_Device, m_IndexBuffer, nullptr );
    vkFreeMemory( m_Device, m_IndexBufferMemory, nullptr );

    vkDestroyBuffer( m_Device, m_VertexBuffer, nullptr );
    vkFreeMemory( m_Device, m_VertexBufferMemory, nullptr );

    vkDestroyCommandPool( m_Device, m_CommandPool, nullptr );

    vkDestroyPipeline( m_Device, m_GraphicsPipeline, nullptr );
    vkDestroyPipelineLayout( m_Device, m_PipelineLayout, nullptr );
    vkDestroyRenderPass( m_Device, m_RenderPass, nullptr );
    

    vkDestroyDevice( m_Device, nullptr );
    vkDestroySurfaceKHR( m_Instance, m_Surface, nullptr );

    if ( enableValidationLayers )
    {
        DestroyDebugUtilsMessengerEXT( m_Instance, debugMessenger, nullptr );
    }

    vkDestroyInstance( m_Instance, nullptr );
    glfwDestroyWindow( m_Window );
    glfwTerminate();
}

void App::drawFrame()
{
    vkWaitForFences( m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX );
    
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR( m_Device, m_SwapChain, UINT64_MAX, m_ImageAvailableSemaphores[m_CurrentFrame],
                                             VK_NULL_HANDLE, &imageIndex );
    if ( result == VK_ERROR_OUT_OF_DATE_KHR )
    {
        recreateSwapChain();
        return;
    }
    else if ( result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR )
    {
        throw std::runtime_error( "failed to acquire swap chain image!" );
    }

    updateUniformBuffer( m_CurrentFrame );

    vkResetFences( m_Device, 1, &m_InFlightFences[m_CurrentFrame] );

    vkResetCommandBuffer( m_CommandBuffers[m_CurrentFrame], /*VkCommandBufferResetFlagBits*/ 0 );
    recordCommandBuffer( m_CommandBuffers[m_CurrentFrame], imageIndex );

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &m_CommandBuffers[m_CurrentFrame];

    VkSemaphore signalSemaphores[] = { m_RenderFinishedSemaphores[m_CurrentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if ( vkQueueSubmit( m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame] ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to submit draw command buffer!" );
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = { m_SwapChain };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR( m_PresentQueue, &presentInfo );

    if ( result == VK_ERROR_OUT_OF_DATE_KHR || 
         result == VK_SUBOPTIMAL_KHR || 
         m_FramebufferResized )
    {
        m_FramebufferResized = false;
        recreateSwapChain();
    }
    else if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to present swap chain image!" );
    }


    m_CurrentFrame = ( m_CurrentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

void App::createInstance()
{
    if ( enableValidationLayers && !checkValidationLayerSupport() )
    {
        throw std::runtime_error( "Validation layers requested, but not available." );
    }

    VkApplicationInfo appInfo{};
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName   = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION( 1, 0, 0 );
    appInfo.apiVersion         = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    auto extensions = getRequiredExtensions();
    createInfo.enabledExtensionCount = static_cast<uint32_t>( extensions.size() );
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if ( enableValidationLayers )
    {
        createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();

        populateDebugMessengerCreateInfo( debugCreateInfo );
        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    }
    else
    {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    VkResult result = vkCreateInstance( &createInfo, nullptr, &m_Instance );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create instance!" );
    }
}

void App::createSurface()
{
    VkResult result = glfwCreateWindowSurface( m_Instance, m_Window, nullptr, &m_Surface );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create window surface." );
    }
}

void App::createSwapChain()
{
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport( m_PhysicalDevice );

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
    VkPresentModeKHR   presentMode   = chooseSwapPresentMode( swapChainSupport.presentModes );
    VkExtent2D         extent        = chooseSwapExtent( swapChainSupport.capabilities );

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if ( swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount )
    {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType                    = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface                  = m_Surface;
    createInfo.minImageCount            = imageCount;
    createInfo.imageFormat              = surfaceFormat.format;
    createInfo.imageColorSpace          = surfaceFormat.colorSpace;
    createInfo.imageExtent              = extent;
    createInfo.imageArrayLayers         = 1;
    createInfo.imageUsage               = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    // It is also possible that you'll render images to a separate image
    // first to perform operations like post-processing.
    // In that case you may use a value like 'VK_IMAGE_USAGE_TRANSFER_DST_BIT'

    QueueFamilyIndices indices              = findQueueFamilies( m_PhysicalDevice );
    uint32_t           queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    if ( indices.graphicsFamily != indices.presentFamily )
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;       // Optional
        createInfo.pQueueFamilyIndices   = nullptr; // Optional
    }

    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode    = presentMode;
    createInfo.clipped        = VK_TRUE;
    createInfo.oldSwapchain   = VK_NULL_HANDLE;

    if ( vkCreateSwapchainKHR( m_Device, &createInfo, nullptr, &m_SwapChain ) )
    {
        throw std::runtime_error( "Failed to create swap chain." );
    }

    vkGetSwapchainImagesKHR( m_Device, m_SwapChain, &imageCount, nullptr );
    m_SwapChainImages.resize( imageCount );
    vkGetSwapchainImagesKHR( m_Device, m_SwapChain, &imageCount, m_SwapChainImages.data() );
    m_SwapChainImageFormat = surfaceFormat.format;
    m_SwapChainExtent      = extent;
}

void App::createImageView()
{
    m_SwapChainImageViews.resize( m_SwapChainImages.size() );
    for ( size_t i = 0; i < m_SwapChainImages.size(); ++i )
    {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image    = m_SwapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format   = m_SwapChainImageFormat;

        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;

        VkResult result = vkCreateImageView( m_Device, &createInfo, nullptr, &m_SwapChainImageViews[i] );
        if ( result != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create image view" );
        }
    }
}

void App::createGraphicsPipeline()
{
    auto vertShaderCode = readFile( "vert.spv" );
    auto fragShaderCode = readFile( "frag.spv" );

    VkShaderModule vertShaderModule = createShaderModule( vertShaderCode );
    VkShaderModule fragShaderModule = createShaderModule( fragShaderCode );

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
    
    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescription = Vertex::getAttributeDescription();

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>( attributeDescription.size() );
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescription.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    std::vector<VkDynamicState> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>( dynamicStates.size() );
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_DescriptorSetLayout;

    if ( vkCreatePipelineLayout( m_Device, &pipelineLayoutInfo, nullptr, &m_PipelineLayout ) != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create pipeline layout!" );
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_PipelineLayout;
    pipelineInfo.renderPass = m_RenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if ( vkCreateGraphicsPipelines( m_Device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_GraphicsPipeline ) !=
         VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create graphics pipeline!" );
    }

    vkDestroyShaderModule( m_Device, fragShaderModule, nullptr );
    vkDestroyShaderModule( m_Device, vertShaderModule, nullptr );
}

void App::createFramebuffers()
{
    m_SwapChainFramebuffers.resize( m_SwapChainImageViews.size() );

    for ( size_t i = 0; i < m_SwapChainImageViews.size(); i++ )
    {
        VkImageView attachments[] = { m_SwapChainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_RenderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_SwapChainExtent.width;
        framebufferInfo.height = m_SwapChainExtent.height;
        framebufferInfo.layers = 1;

        if ( vkCreateFramebuffer( m_Device, &framebufferInfo, nullptr, &m_SwapChainFramebuffers[i] ) != VK_SUCCESS )
        {
            throw std::runtime_error( "failed to create framebuffer!" );
        }
    }
}

void App::createCommandPool()
{
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies( m_PhysicalDevice );

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();
    if ( vkCreateCommandPool( m_Device, &poolInfo, nullptr, &m_CommandPool ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create command pool!" );
    }

}

void App::createVertexBuffer()
{
    VkDeviceSize bufferSize = sizeof( rectangle[0] ) * rectangle.size();
    
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer( bufferSize,
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                  stagingBuffer,
                  stagingBufferMemory );

    void *data;
    vkMapMemory( m_Device, stagingBufferMemory, 0, bufferSize, 0, &data );
    memcpy( data, rectangle.data(), (size_t)bufferSize );
    vkUnmapMemory( m_Device, stagingBufferMemory );

    createBuffer( bufferSize, 
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_VertexBuffer, m_VertexBufferMemory );
    
    copyBuffer( stagingBuffer, m_VertexBuffer, bufferSize );
    vkDestroyBuffer( m_Device, stagingBuffer, nullptr );
    vkFreeMemory( m_Device, stagingBufferMemory, nullptr );
}

void App::createIndexBuffer()
{
    VkDeviceSize bufferSize = sizeof( indices[0] ) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer( bufferSize, 
                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                  stagingBuffer, stagingBufferMemory );

    void *data;
    vkMapMemory( m_Device, stagingBufferMemory, 0, bufferSize, 0, &data );
    memcpy( data, indices.data(), (size_t)bufferSize );
    vkUnmapMemory( m_Device, stagingBufferMemory );

    createBuffer( bufferSize, 
                  VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
                  m_IndexBuffer, m_IndexBufferMemory );

    copyBuffer( stagingBuffer, m_IndexBuffer, bufferSize );

    vkDestroyBuffer( m_Device, stagingBuffer, nullptr );
    vkFreeMemory( m_Device, stagingBufferMemory, nullptr );

}

void App::createUniformBuffers()
{
    VkDeviceSize bufferSize = sizeof( UniformBufferObject );

    m_UniformBuffers.resize( MAX_FRAMES_IN_FLIGHT );
    m_UniformBuffersMemory.resize( MAX_FRAMES_IN_FLIGHT );
    m_UniformBuffersMapped.resize( MAX_FRAMES_IN_FLIGHT );

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        createBuffer( bufferSize, 
                      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
                      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                      m_UniformBuffers[i],
                      m_UniformBuffersMemory[i] );

        vkMapMemory( m_Device, 
                     m_UniformBuffersMemory[i], 
                     0, bufferSize, 0, 
                     &m_UniformBuffersMapped[i] );

    }
}

void App::createDescriptorPool()
{
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT );

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT );

    VkResult result = vkCreateDescriptorPool( m_Device, &poolInfo, nullptr, &m_DescriptorPool );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create Descriptor pool." );
    }
}

void App::createDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts( MAX_FRAMES_IN_FLIGHT, 
                                                m_DescriptorSetLayout );
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorSetCount = static_cast<uint32_t>( MAX_FRAMES_IN_FLIGHT );
    allocInfo.descriptorPool = m_DescriptorPool;
    allocInfo.pSetLayouts = layouts.data();

    m_DescriptorSets.resize( MAX_FRAMES_IN_FLIGHT );
    VkResult result = vkAllocateDescriptorSets( m_Device, 
                                                &allocInfo, 
                                                m_DescriptorSets.data() );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to allocae descriptor sets" );
    }

    for ( size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_UniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof( UniformBufferObject );
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_DescriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        descriptorWrite.pImageInfo = nullptr; // Optional
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets( m_Device, 1, &descriptorWrite, 0, nullptr );
    }

}

void App::createCommandBuffers()
{
    if ( m_CommandBuffers.size() < MAX_FRAMES_IN_FLIGHT )
        m_CommandBuffers.resize( MAX_FRAMES_IN_FLIGHT );
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_CommandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>( m_CommandBuffers.size() ); // OPTIONAL
    if ( vkAllocateCommandBuffers( m_Device, &allocInfo, m_CommandBuffers.data() ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate command buffers!" );
    }
    
}

void App::createSyncObjects()
{
    if ( m_ImageAvailableSemaphores.size() < MAX_FRAMES_IN_FLIGHT )
        m_ImageAvailableSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    if ( m_RenderFinishedSemaphores.size() < MAX_FRAMES_IN_FLIGHT )
        m_RenderFinishedSemaphores.resize( MAX_FRAMES_IN_FLIGHT );
    if ( m_InFlightFences.size() < MAX_FRAMES_IN_FLIGHT )
        m_InFlightFences.resize( MAX_FRAMES_IN_FLIGHT );

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for ( int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
    {
        if ( vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i] ) != VK_SUCCESS ||
             vkCreateSemaphore( m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i] ) != VK_SUCCESS ||
             vkCreateFence( m_Device, &fenceInfo, nullptr, &m_InFlightFences[i] ) != VK_SUCCESS )
        {
            throw std::runtime_error( "Failed to create synchronization objects for a frame.!" );
        }
    }
}

void App::recreateSwapChain()
{
    int width = 0, height = 0;
    glfwGetFramebufferSize( m_Window, &width, &height );
    while ( width == 0 || height == 0 )
    {
        glfwGetFramebufferSize( m_Window, &width, &height );
        glfwWaitEvents();
    }

    vkDeviceWaitIdle( m_Device );

    cleanupSwapchain();

    createSwapChain();
    createImageView();
    createFramebuffers();
}

void App::cleanupSwapchain()
{
    size_t i = 0;
    for ( i = 0; i < m_SwapChainFramebuffers.size(); ++i )
    {
        vkDestroyFramebuffer( m_Device, m_SwapChainFramebuffers[i], nullptr );
    }
    for ( i = 0; i < m_SwapChainImageViews.size(); ++i )
    {
        vkDestroyImageView( m_Device, m_SwapChainImageViews[i], nullptr );
    }
    vkDestroySwapchainKHR( m_Device, m_SwapChain, nullptr );
}

void App::updateUniformBuffer( uint32_t currentImage )
{
    static auto startTime = std::chrono::high_resolution_clock::now();

    auto currentTime = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>( currentTime - startTime ).count();

    UniformBufferObject ubo{};
    ubo.model = glm::rotate( glm::mat4( 1.0f ), time * glm::radians( 90.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
    ubo.view =
        glm::lookAt( glm::vec3( 2.0f, 2.0f, 2.0f ), glm::vec3( 0.0f, 0.0f, 0.0f ), glm::vec3( 0.0f, 0.0f, 1.0f ) );
    ubo.proj =
        glm::perspective( glm::radians( 45.0f ), m_SwapChainExtent.width / (float)m_SwapChainExtent.height, 0.1f, 10.0f );
    ubo.proj[1][1] *= -1;

    memcpy( m_UniformBuffersMapped[currentImage], &ubo, sizeof( ubo ) );
}

void App::setupDebugMessenger()
{
    if ( !enableValidationLayers )
        return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo( createInfo );

    if ( CreateDebugUtilsMessengerEXT( m_Instance, &createInfo, nullptr, &debugMessenger ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to set up debug messenger!" );
    }
}


void App::createRenderPass()
{
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = m_SwapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;

    if ( vkCreateRenderPass( m_Device, &renderPassInfo, nullptr, &m_RenderPass ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create render pass!" );
    }
}

void App::createDescriptorSetLayout()
{
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    VkResult result = vkCreateDescriptorSetLayout( m_Device, &layoutInfo, nullptr, &m_DescriptorSetLayout );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create descriptor set layout. " );
    }
}

void App::pickphysicalDevice()
{
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices( m_Instance, &deviceCount, nullptr );
    if ( deviceCount == 0 )
    {
        throw std::runtime_error( "Failed to find GPUs with Vulkan support." );
    }

    std::vector<VkPhysicalDevice> devices( deviceCount );
    vkEnumeratePhysicalDevices( m_Instance, &deviceCount, devices.data() );
    for ( const auto &m_Device : devices )
    {
        if ( isDeviceSuitable( m_Device ) )
        {
            m_PhysicalDevice = m_Device;
            break;
        }
    }
    if ( m_PhysicalDevice == VK_NULL_HANDLE )
    {
        throw std::runtime_error( "Failed to find a suitable GPU." );
    }
}

bool App::isDeviceSuitable( VkPhysicalDevice device )
{
    QueueFamilyIndices indices = findQueueFamilies( device );

    bool extensionsSupported = checkDeviceExtensionSupport( device );

    bool swapChainAdequate = false;
    if ( extensionsSupported )
    {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
        swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    }

    return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices App::findQueueFamilies( VkPhysicalDevice device )
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

    std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
    vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

    int i = 0;
    for ( const auto &queueFamily : queueFamilies )
    {
        if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
        {
            indices.graphicsFamily = i;
        }

        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR( device, i, m_Surface, &presentSupport );

        if ( presentSupport )
        {
            indices.presentFamily = i;
        }

        if ( indices.isComplete() )
        {
            break;
        }

        i++;
    }

    return indices;
}

bool App::checkDeviceExtensionSupport( VkPhysicalDevice device )
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, nullptr );

    std::vector<VkExtensionProperties> availableExtensions( extensionCount );

    vkEnumerateDeviceExtensionProperties( device, nullptr, &extensionCount, availableExtensions.data() );

    std::set<std::string> requiredExtension( deviceExtensions.begin(), deviceExtensions.end() );

    for ( const auto &extension : availableExtensions )
    {
        requiredExtension.erase( extension.extensionName );
    }

    return requiredExtension.empty();
}

void App::createBuffer( VkDeviceSize size,
                        VkBufferUsageFlags usage,
                        VkMemoryPropertyFlags properties,
                        VkBuffer &buffer,
                        VkDeviceMemory &bufferMemory )
{
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if ( vkCreateBuffer( m_Device, &bufferInfo, nullptr, &buffer ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create buffer!" );
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements( m_Device, buffer, &memRequirements );

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType( memRequirements.memoryTypeBits, properties );

    if ( vkAllocateMemory( m_Device, &allocInfo, nullptr, &bufferMemory ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to allocate buffer memory!" );
    }

    vkBindBufferMemory( m_Device, buffer, bufferMemory, 0 );
}

VkShaderModule App::createShaderModule( const std::vector<char> &code )
{
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t *>( code.data() );
    VkShaderModule shaderModule;
    if ( vkCreateShaderModule( m_Device, &createInfo, nullptr, &shaderModule ) != VK_SUCCESS )
    {
        throw std::runtime_error( "failed to create shader module!" );
    }

    return shaderModule;
}

SwapChainSupportDetails App::querySwapChainSupport( VkPhysicalDevice device )
{
    SwapChainSupportDetails details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, m_Surface, &details.capabilities );

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR( device, m_Surface, &formatCount, nullptr );
    if ( formatCount != 0 )
    {
        details.formats.resize( formatCount );
        vkGetPhysicalDeviceSurfaceFormatsKHR( device, m_Surface, &formatCount, details.formats.data() );
    }

    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR( device, m_Surface, &presentModeCount, nullptr );
    if ( presentModeCount != 0 )
    {
        details.presentModes.resize( presentModeCount );
        vkGetPhysicalDeviceSurfacePresentModesKHR( device, m_Surface, &presentModeCount, details.presentModes.data() );
    }

    return details;
}

VkSurfaceFormatKHR App::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats ) const
{
    if ( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
        return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };

    for ( const auto &availableFormat : availableFormats )
    {
        if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
             availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
        {
            return availableFormat;
        }
    }

    return availableFormats[0];
}

VkPresentModeKHR App::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes ) const
{
    VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

    for ( const auto &availablePresentMode : availablePresentModes )
    {
        if ( availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
        {
            bestMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
        else if ( availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR )
        {
            bestMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        }
    }
    return bestMode;
}

VkExtent2D App::chooseSwapExtent( const VkSurfaceCapabilitiesKHR &capabilities ) const
{
    if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
    {
        return capabilities.currentExtent;
    }
    else
    {
        VkExtent2D actualExtent = { WIN_WIDTH, WIN_HEIGHT };

        // capabilitiesMin < actualExtentSize < capabilitiesMax
        actualExtent.width  = std::max( capabilities.minImageExtent.width,
                                        std::min( actualExtent.width, capabilities.maxImageExtent.width ) );
        actualExtent.height = std::max( capabilities.minImageExtent.height,
                                        std::min( actualExtent.height, capabilities.maxImageExtent.height ) );
        return actualExtent;
    }
    return VkExtent2D();
}

void App::createLogicalDevice()
{
    QueueFamilyIndices indices = findQueueFamilies( m_PhysicalDevice );

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

    /*VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
    queueCreateInfo.queueCount = 1;*/

    float queuePriority = 1.0f;
    for ( uint32_t queueFamily : uniqueQueueFamilies )
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back( queueCreateInfo );
    }
    // queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures{};

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
    createInfo.pQueueCreateInfos    = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    // createInfo.enabledExtensionCount = 0;
    createInfo.enabledExtensionCount   = static_cast<uint32_t>( deviceExtensions.size() );
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if ( enableValidationLayers )
    {
        createInfo.enabledLayerCount   = static_cast<uint32_t>( validationLayers.size() );
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    else
    {
        createInfo.enabledLayerCount = 0;
    }

    VkResult result = vkCreateDevice( m_PhysicalDevice, &createInfo, nullptr, &m_Device );
    if ( result != VK_SUCCESS )
    {
        throw std::runtime_error( "Failed to create logical device. " );
    }

    vkGetDeviceQueue( m_Device, indices.graphicsFamily.value(), 0, &m_GraphicsQueue );
    vkGetDeviceQueue( m_Device, indices.presentFamily.value(), 0, &m_PresentQueue );
}

bool App::checkValidationLayerSupport() const
{
    // to list the available layers.
    uint32_t layerCount = 0;
    vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

    std::vector<VkLayerProperties> availableLayers( layerCount );
    vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

    // Check if all of the layers in validationLayers
    bool layerFound = false;
    for ( const char *layerName : validationLayers )
    {
        layerFound = false;

        for ( const auto &layerProperties : availableLayers )
        {
            if ( strcmp( layerName, layerProperties.layerName ) == 0 )
            {
                layerFound = true;
                break;
            }
        }

        if ( !layerFound )
            return false;
    }

    return true;
}