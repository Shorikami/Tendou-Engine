#ifndef GLOBALS_H
#define GLOBALS_H

#include "../Rendering/Camera.h"
#include <GLFW/glfw3.h>

namespace Drevis
{
	static constexpr int WIDTH = 1280;
	static constexpr int HEIGHT = 720;

	static Camera c(glm::vec3(-4.0f, 0.0f, 2.0f));

	static float lastX = 0.0f;
	static float lastY = 0.0f;
	static bool firstMouse = true;
	static void MouseCallback(GLFWwindow* win, double x, double y);
}

#endif