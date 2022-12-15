#ifndef SIMPLESCENE_H
#define SIMPLESCENE_H

#include "Scene.h"

#include "../../Rendering/Texture.h"
#include "../../Rendering/UniformBuffer.hpp"

#include <memory>
#include <vector>

namespace Tendou
{
	class SimpleScene : public Scene
	{
	public:
		SimpleScene(Window& window, TendouDevice& device);
		~SimpleScene() override;

		int Init() override;
		int PreUpdate() override;
		int Update() override;
		int PostUpdate() override;
	private:
		void LoadGameObjects();

		std::unique_ptr<UniformBuffer<WorldUBO>> worldUBO;
		std::unique_ptr<UniformBuffer<LightsUBO>> lightUBO;
		std::vector<std::unique_ptr<Texture>> textures;
	};
}

#endif