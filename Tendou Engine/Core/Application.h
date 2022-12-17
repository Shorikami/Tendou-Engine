#ifndef APPLICATION_H
#define APPLICATION_H

#include "Window.h"

#include "../Vulkan/Descriptor.h"
#include "../Vulkan/TendouDevice.h"

#include "../Editor/Editor.h"

#include "../Components/GameObject.h"
#include "../Rendering/Scenes/Scene.h"

#include "../Rendering/UniformBuffer.hpp"

#include <memory>
#include <vector>

namespace Tendou
{
	static constexpr int WIDTH = 1280;
	static constexpr int HEIGHT = 720;

	class Application
	{
	public:
		Application();
		~Application();

		Application(const Application&) = delete;
		Application& operator=(const Application&) = delete;

		void Run();

	private:
		Window appWindow{ Tendou::WIDTH, Tendou::HEIGHT, "Tendou Engine" };
		TendouDevice device{ appWindow };

		std::unique_ptr<Editor> editor;
		std::unique_ptr<Scene> scene;
	};
}

#endif