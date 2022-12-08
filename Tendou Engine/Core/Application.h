#ifndef APPLICATION_H
#define APPLICATION_H

#include "Window.h"
#include "Globals.h"

#include "../Vulkan/TendouDevice.h"

#include "../Rendering/Scene.h"
#include "../Rendering/Descriptor.h"

#include "../Components/GameObject.h"

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
		void InitImGUI();
		void LoadGameObjects();

		Window appWindow{ Tendou::WIDTH, Tendou::HEIGHT, "Drevis Engine" };
		TendouDevice device{ appWindow };
		Scene scene{ appWindow, device };

		VkPipelineLayout layout;
		VkDescriptorPool imguiPool; // todo: abstract this later

		// Note: order of declarations matters - need the global pool to be destroyed
		// before the device
		std::unique_ptr<DescriptorPool> globalPool{};
		GameObject::Map gameObjects;
		Camera c;
	};
}

#endif