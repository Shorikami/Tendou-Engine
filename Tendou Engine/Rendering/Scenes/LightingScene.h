#ifndef LIGHTINGSCENE_H
#define LIGHTINGSCENE_H

#include "Scene.h"

#include "../../Rendering/Texture.h"
#include "../../Rendering/UniformBuffer.hpp"

#include <memory>
#include <vector>

#define MAX_LIGHTS 16

namespace Tendou
{
	class LightingScene : public Scene
	{
	public:
		struct 
		{
			int currLights = 1;
			float sphereLineRad = 30.0f;
			bool rotateSpheres = true;

			glm::vec2 nearFar = glm::vec2(0.1f, 20.0f);
			glm::vec3 attenuation = glm::vec3(0.5f, 0.37f, 0.2f);
			glm::vec3 lightCoeffs = glm::vec3(1.0f);
			glm::vec3 emissive = glm::vec3(0.0f);

			glm::vec3 ambient[16] = {};
			glm::vec3 diffuse[16] = {};
			glm::vec3 specular[16] = {};

		} editorVars;

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