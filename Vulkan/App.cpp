#include "App.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <optional>

void HelloTriangleApplication::run()
{
	initWindow();
	initVulkan();
	mainLoop();
	cleanup();
}

void HelloTriangleApplication::initWindow()
{
	// These built-in functions are 22 the first step to the necessary. 
	glfwInit(); asdasda
	// GLFW was originally desinged to create an OpenGL context.
	// we need to tell it to not create an OpenGL context with a subsequent call
	glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
	// to Disable the configuration a resize windows. 
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	window = glfwCreateWindow( WIN_WIDTH, WIN_HEIGHT,
							   "Draw Triangle",
							   nullptr, nullptr );
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
	// TODO: setupDebugMessenger(); 
	// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Validation_layers
	pickphysicalDevice();
	createLogicalDevice();
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
	vkDestroyDevice( device, nullptr );
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

	return indices.isComplete();
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

		if ( indices.isComplete() )
		{
			break;
		}
		i++;
	}

	return indices;
}

void HelloTriangleApplication::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies( physicalDevice );

	VkDeviceQueueCreateInfo queueCreateInfo{};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo.queueFamilyIndex = indices.graphicsFamily.value();
	queueCreateInfo.queueCount = 1;

	float queuePriority = 1.0f;
	queueCreateInfo.pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures{};

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.pQueueCreateInfos = &queueCreateInfo;
	createInfo.queueCreateInfoCount = 1;

	createInfo.pEnabledFeatures = &deviceFeatures;

	createInfo.enabledExtensionCount = 0;

	if ( enableValidationLayers )
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>( validationLayers.size() );
		createInfo.ppEnabledExtensionNames = validationLayers.data();
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


