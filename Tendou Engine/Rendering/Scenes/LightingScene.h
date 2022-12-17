#ifndef LIGHTINGSCENE_H
#define LIGHTINGSCENE_H

#include "Scene.h"

#include "../../Rendering/Texture.h"
#include "../../Rendering/UniformBuffer.hpp"

#include <memory>
#include <vector>

#define CURR_LIGHTS 8

namespace Tendou
{
	class LightingScene : public Scene
	{
	public:
		struct EditorVariables
		{

		};

		LightingScene(Window& window, TendouDevice& device);
		~LightingScene() override;

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