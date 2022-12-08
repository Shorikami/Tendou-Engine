#ifndef WINDOW_HPP
#define WINDOW_HPP

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Tendou
{
	class Window
	{
	public:
		Window(int w, int h, std::string name);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		__inline bool ShouldClose() { return glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS 
			&& glfwWindowShouldClose(window) == 0; };
		__inline VkExtent2D GetExtent() { return { static_cast<uint32_t>(width), static_cast<uint32_t>(height) }; }
		__inline bool WasWindowResized() { return frameBufferResized; }
		__inline void ResetWindowResizedFlag() { frameBufferResized = false; }

		void CreateWindowSurface(VkInstance instance, VkSurfaceKHR* surface);
		GLFWwindow* GetGLFWwindow() { return window; }

	private:
		static void FramebufferResizeCallback(GLFWwindow* window, int w, int h);
		void InitWindow();

		int width;
		int height;
		bool frameBufferResized = false;

		std::string windowName;
		GLFWwindow* window;
	};
}

#endif