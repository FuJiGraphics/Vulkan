#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 

const uint32_t WIN_WIDTH  = 800;
const uint32_t WIN_HEIGHT = 600;

class HelloTriangleApplication
{
public:
	void run();

private:
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

private:
	void createInstance();

private: // Window Application
	GLFWwindow* window = nullptr;

private: // Vulkan API
	VkInstance instance;
};
