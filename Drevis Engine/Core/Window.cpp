#include "Window.h"
#include "Globals.h"

#include <stdexcept>

namespace Drevis
{

	Window::Window(int w, int h, std::string name)
		: width(w)
		, height(h)
		, windowName(name)
	{
		InitWindow();
	}

	Window::~Window()
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

	void Window::InitWindow()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		window = glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);

		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, FramebufferResizeCallback);
	}

	void Window::CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface)
	{
		if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
		{
			throw std::runtime_error("Failed to create window surface!");
		}
	}

	void Window::FramebufferResizeCallback(GLFWwindow* window, int w, int h)
	{
		auto wWindow = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
		wWindow->frameBufferResized = true;
		wWindow->width = w;
		wWindow->height = h;
		c.currW = w;
		c.currH = h;
	}
}