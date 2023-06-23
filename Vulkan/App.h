#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 
#include <vector>

const uint32_t WIN_WIDTH  = 800;
const uint32_t WIN_HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

#ifdef NDEBUG
const bool enableVaildationLayers = false;
#else
const bool enableVaildationLayers = true;
#endif 

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
	bool checkValidationLayerSupport();
	// TODO: 메시지 콜백 레이어 활성화

private:
	void createInstance();

private: // Window Application
	GLFWwindow* window = nullptr;

private: // Vulkan API
	VkInstance instance;
};
