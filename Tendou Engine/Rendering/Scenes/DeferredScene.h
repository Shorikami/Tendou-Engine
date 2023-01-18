#ifndef DEFERREDSCENE_H
#define DEFERREDSCENE_H

#include "Scene.h"

#include "../../Rendering/Texture.h"
#include "../../Rendering/UniformBuffer.hpp"

#include <memory>
#include <vector>

namespace Tendou
{
#define MAX_LIGHTS 10

	class DeferredScene : public Scene
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

		DeferredScene(Window& window, TendouDevice& device);
		~DeferredScene() override;

		int Init() override;
		int PreUpdate() override;
		int Update() override;
		int PostUpdate() override;

		int Render(VkCommandBuffer buf, FrameInfo& f) override;


		

	private:
		void LoadGameObjects();

		void CreateUBOs();
		void CreateSetLayouts();
		void CreateRenderPasses();
		void CreateRenderSystems();

		std::unique_ptr<UniformBuffer<WorldUBO>> worldUBO;
		std::unique_ptr<UniformBuffer<LightPassUBO>> lightingPass;
		std::vector<std::unique_ptr<Texture>> textures;

		GameObject::Map localLights;
		Tendou::Light lightValues[MAX_LIGHTS];
	};
}

#endif