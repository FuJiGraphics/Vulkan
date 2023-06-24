#pragma once
// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 
#include <vector>
#include <optional>

const uint32_t WIN_WIDTH  = 800;
const uint32_t WIN_HEIGHT = 600;

const std::vector<const char*> validationLayers =
{
	"VK_LAYER_KHRONOS_validation"
};

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;

	bool isComplete() const
	{
		return graphicsFamily.has_value();
	}
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
	void initWindow();
	void initVulkan();
	void mainLoop();
	void cleanup();

private:
	bool checkValidationLayerSupport() const;
	// TODO: 메시지 콜백 레이어 활성화

private:
	void createInstance();

private: // Physical Devices and Queue Families
	void pickphysicalDevice();
	bool isDeviceSuitable( VkPhysicalDevice device );
	QueueFamilyIndices findQueueFamilies( VkPhysicalDevice device );

private: // Logical Device and queues
	void createLogicalDevice();

private: // Window Application
	GLFWwindow* window = nullptr;

private: // Vulkan API
	VkInstance instance             = VK_NULL_HANDLE;
	VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	VkDevice device					= VK_NULL_HANDLE;
	VkQueue graphicsQueue			= VK_NULL_HANDLE;
};
