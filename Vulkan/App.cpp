#include "App.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>

void HelloTriangleApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void HelloTriangleApplication::initWindow()
{
	// These built-in functions are the first step to the necessary. 
	glfwInit();
	// GLFW was originally desinged to create an OpenGL context.
	// we need to tell it to not create an OpenGL context with a subsequent call
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	// to Disable the configuration a resize windows. 
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	window = glfwCreateWindow( gWinWidth, gWinHeight,
							   "Draw Triangle",
							   nullptr, nullptr );
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
	// TODO: setupDebugMessenger(); 
	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
	createSurface();
	pickphysicalDevice();
	createLogicalDevice();
	createSwapChain();
}

void HelloTriangleApplication::mainLoop()
{
	while ( !glfwWindowShouldClose( window ) )
	{
		glfwPollEvents();
	}
}

void HelloTriangleApplication::cleanup()
{
	vkDestroySwapchainKHR( device, swapChain, nullptr );
	vkDestroyDevice( device, nullptr );
	vkDestroySurfaceKHR( instance, surface, nullptr );
	vkDestroyInstance( instance, nullptr );
	glfwDestroyWindow( window );
	glfwTerminate();
}

void HelloTriangleApplication::createInstance()
{
	if ( enableValidationLayers && !checkValidationLayerSupport() )
	{
		throw std::runtime_error( "Validation layers requested, but not available." );
	}

	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "Hello Triangle";
	appInfo.applicationVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.pEngineName = "No Engine";
	appInfo.engineVersion = VK_MAKE_VERSION( 1, 0, 0 );
	appInfo.apiVersion = VK_API_VERSION_1_0;

	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;


	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );
	if ( result != VK_SUCCESS )
	{
		throw std::runtime_error( "failed to create instance!" );
	}
}

void HelloTriangleApplication::createSurface()
{
	VkResult result = glfwCreateWindowSurface( instance, window, nullptr, &surface );
	if ( result != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create window surface." );
	}
}

void HelloTriangleApplication::createSwapChain()
{
	SwapChainSupportDetails swapChainSupport = querySwapChainSupport( physicalDevice );

	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat( swapChainSupport.formats );
	VkPresentModeKHR presentMode = chooseSwapPresentMode( swapChainSupport.presentModes );
	VkExtent2D extent = chooseSwapExtent( swapChainSupport.capabilities );

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if ( swapChainSupport.capabilities.maxImageCount > 0 &&
		 imageCount > swapChainSupport.capabilities.maxImageCount )
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	// It is also possible that you'll render images to a separate image 
	// first to perform operations like post-processing. 
	// In that case you may use a value like 'VK_IMAGE_USAGE_TRANSFER_DST_BIT'

	QueueFamilyIndices indices = findQueueFamilies( physicalDevice );
	uint32_t queueFamilyIndices[] = {
		indices.graphicsFamily.value(),
		indices.presentFamily.value()
	};
	
	if ( indices.graphicsFamily != indices.presentFamily )
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if ( vkCreateSwapchainKHR( device, &createInfo, nullptr, &swapChain ) )
	{
		throw std::runtime_error( "Failed to create swap chain." );
	}

	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, nullptr );
	swapChainImages.resize( imageCount );
	vkGetSwapchainImagesKHR( device, swapChain, &imageCount, swapChainImages.data() );
	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

}

void HelloTriangleApplication::pickphysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices( instance, &deviceCount, nullptr );
	if ( deviceCount == 0 )
	{
		throw std::runtime_error( "Failed to find GPUs with Vulkan support." );
	}

	std::vector<VkPhysicalDevice> devices( deviceCount );
	vkEnumeratePhysicalDevices( instance, &deviceCount, devices.data() );
	for ( const auto& device : devices )
	{
		if ( isDeviceSuitable( device ) )
		{
			physicalDevice = device;
			break;
		}
	}
	if ( physicalDevice == VK_NULL_HANDLE )
	{
		throw std::runtime_error( "Failed to find a suitable GPU." );
	}
}

bool HelloTriangleApplication::isDeviceSuitable( VkPhysicalDevice device )
{
	// TODO : check a suitable base device

	// like the name, type and Vulkan version
	/*VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties( device, &deviceProperties );*/

	// like texture compression, 64bit floats, multi viewport rendering
	/*VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures( device, &deviceFeatures );*/
	QueueFamilyIndices indices = findQueueFamilies( device );

	bool extensionsSupported = checkDeviceExtensionSupport( device );

	bool swapChainAdequate = false;
	if ( extensionsSupported )
	{
		SwapChainSupportDetails swapChainSupport = querySwapChainSupport( device );
		swapChainAdequate = !swapChainSupport.formats.empty() && 
							!swapChainSupport.presentModes.empty();
	}

	return indices.isComplete() && extensionsSupported && swapChainAdequate;
}

QueueFamilyIndices HelloTriangleApplication::findQueueFamilies( VkPhysicalDevice device )
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, nullptr );

	std::vector<VkQueueFamilyProperties> queueFamilies( queueFamilyCount );
	vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies.data() );

	int i = 0;
	for ( const auto& queueFamily : queueFamilies )
	{
		if ( queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			indices.graphicsFamily = i;
		}

		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );

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

bool HelloTriangleApplication::checkDeviceExtensionSupport( VkPhysicalDevice device )
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties( device, 
										  nullptr, 
										  &extensionCount, 
										  nullptr );

	std::vector<VkExtensionProperties> availableExtensions( extensionCount );

	vkEnumerateDeviceExtensionProperties( device, 
										  nullptr, 
										  &extensionCount, 
										  availableExtensions.data() );

	std::set<std::string> requiredExtension( deviceExtensions.begin(), 
											 deviceExtensions.end() );

	for ( const auto& extension : availableExtensions )
	{
		requiredExtension.erase( extension.extensionName );
	}

	return requiredExtension.empty();
}

SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport( VkPhysicalDevice device )
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device, 
											   surface, 
											   &details.capabilities );

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR( device, 
										  surface, 
										  &formatCount, 
										  nullptr );
	if ( formatCount != 0 )
	{
		details.formats.resize( formatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR( device,
											  surface,
											  &formatCount,
											  details.formats.data() );
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR( device,
											   surface,
											   &presentModeCount,
											   nullptr );
	if ( presentModeCount != 0 )
	{
		details.presentModes.resize( presentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR( device,
												   surface,
												   &presentModeCount,
												   details.presentModes.data() );
	}


	return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat( 
	const std::vector<VkSurfaceFormatKHR>& availableFormats ) const
{
	if ( availableFormats.size() == 1 && availableFormats[0].format == VK_FORMAT_UNDEFINED )
		return {VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR};

	for ( const auto& availableFormat : availableFormats )
	{
		if ( availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
			 availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR )
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode( const std::vector<VkPresentModeKHR>& availablePresentModes ) const
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for ( const auto& availablePresentMode : availablePresentModes )
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

VkExtent2D HelloTriangleApplication::chooseSwapExtent( const VkSurfaceCapabilitiesKHR& capabilities ) const
{
	if ( capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max() )
	{
		return capabilities.currentExtent;
	}
	else
	{
		VkExtent2D actualExtent = { gWinWidth, gWinHeight };

		// capabilitiesMin < actualExtentSize < capabilitiesMax
		actualExtent.width = std::max( capabilities.minImageExtent.width,
									   std::min( actualExtent.width, capabilities.maxImageExtent.width ) );
		actualExtent.height = std::max( capabilities.minImageExtent.height,
									   std::min( actualExtent.height, capabilities.maxImageExtent.height ) );
		return actualExtent;
	}
	return VkExtent2D();
}

void HelloTriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(),
											   indices.presentFamily.value() };


	/*VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;*/

	float queuePriority = 1.0f;
	for ( uint32_t queueFamily : uniqueQueueFamilies )
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back( queueCreateInfo );
	}
	//queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

	createInfo.queueCreateInfoCount = static_cast<uint32_t>( queueCreateInfos.size() );
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	
	createInfo.pEnabledFeatures = &deviceFeatures;

	// createInfo.enabledExtensionCount = 0;
	createInfo.enabledExtensionCount = static_cast<uint32_t>( deviceExtensions.size() );
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else
	{
		createInfo.enabledLayerCount = 0;
	}

	VkResult result = vkCreateDevice( physicalDevice, &createInfo, nullptr, &device );
	if ( result != VK_SUCCESS )
	{
		throw std::runtime_error( "Failed to create logical device. " );
	}

	vkGetDeviceQueue( device, indices.graphicsFamily.value(), 0, &graphicsQueue );
	vkGetDeviceQueue( device, indices.presentFamily.value(), 0, &presentQueue );
}

bool HelloTriangleApplication::checkValidationLayerSupport() const
{
	// to list the available layers.
	uint32_t layerCount = 0;
	vkEnumerateInstanceLayerProperties( &layerCount, nullptr );

	std::vector<VkLayerProperties> availableLayers( layerCount );
	vkEnumerateInstanceLayerProperties( &layerCount, availableLayers.data() );

	// Check if all of the layers in validationLayers
	bool layerFound = false;
	for ( const char* layerName : validationLayers )
	{
		layerFound = false;

		for ( const auto& layerProperties : availableLayers )
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


