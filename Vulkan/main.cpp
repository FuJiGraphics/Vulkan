// Code reference by Vulkan Tutorial
// https://vulkan-tutorial.com/Drawing_a_triangle/Setup/Base_code

// This header includes the Vulkan header automatically.
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h> 

#include <iostream>
#include <stdexcept>
#include <cstdlib>

class HelloTriangleApplication
{
public:
	void run()
	{
		initWindow();
		initVulkan();
		mainLoop();
		cleanup();
	}

private:
	void initWindow()
	{
		// These built-in functions are the first step to the necessary. 
		glfwInit();
		// GLFW was originally desinged to create an OpenGL context.
		// we need to tell it to not create an OpenGL context with a subsequent call
		glfwWindowHint( GLFW_CLIENT_API, GLFW_NO_API );
		// to Disable the configuration for a resize windows. 
		glfwWindowHint( GLFW_RESIZABLE, GLFW_FALSE );

		window = glfwCreateWindow( m_WinWidth, m_WinHeight,
								   "Draw Triangle",
								   nullptr, nullptr );
	}

	void initVulkan()
	{
		// TODO: Write Vulkan initialize logic here
	}

	void mainLoop()
	{
		while ( !glfwWindowShouldClose( window ) )
		{
			glfwPollEvents();
		}
	}

	void cleanup()
	{
		glfwDestroyWindow( window );
		glfwTerminate();
	}

private:
	const uint32_t m_WinWidth  = 800;
	const uint32_t m_WinHeight = 600;
	GLFWwindow* window         = nullptr;

};

int main()
{
	HelloTriangleApplication app;

	try
	{
		app.run();
	}
	catch ( const std::exception& e )
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}