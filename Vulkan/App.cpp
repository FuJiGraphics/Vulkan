#include "App.h"

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

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
	// to Disable the configuration for a resize windows. 
	glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

	window = glfwCreateWindow( WIN_WIDTH, WIN_HEIGHT,
							   "Draw Triangle",
							   nullptr, nullptr );
}

void HelloTriangleApplication::initVulkan()
{
	createInstance();
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
	vkDestroyInstance( instance, nullptr );
	glfwDestroyWindow( window );
	glfwTerminate();
}

void HelloTriangleApplication::createInstance()
{
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

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;

	glfwExtensions = glfwGetRequiredInstanceExtensions( &glfwExtensionCount );

	createInfo.enabledExtensionCount = glfwExtensionCount;
	createInfo.ppEnabledExtensionNames = glfwExtensions;
	createInfo.enabledLayerCount = 0;

	VkResult result = vkCreateInstance( &createInfo, nullptr, &instance );
}