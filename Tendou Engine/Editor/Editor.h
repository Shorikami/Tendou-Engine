#ifndef EDITOR_H
#define EDITOR_H

#include "../Core/Window.h"
#include "../Rendering/Scenes/Scene.h"

namespace Tendou
{
	class Editor
	{
	public:
		Editor(Window& w, Scene* s, TendouDevice& td);
		~Editor();

		Editor(const Editor&) = delete;
		Editor& operator=(const Editor&) = delete;

		void Setup();
		void Draw(VkCommandBuffer buf);
	private:
		VkDescriptorPool imguiPool;
		TendouDevice& td;
		Scene* activeScene;
	};
}

#endif