#ifndef GLTFSCENE_H
#define GLTFSCENE_H

#include "Scene.h"


#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <tiny_gltf.h>

#include "../../Rendering/Texture.h"
#include "../../Rendering/UniformBuffer.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <memory>
#include <vector>


// Based off Sascha Willems' GLTF scene rendering
// https://gitlab.steamos.cloud/jupiter/vulkan-examples/-/tree/gltf/examples/gltfscenerendering
namespace Tendou
{
	class GLTF
	{
	public:
		TendouDevice& device_;

		struct Vertex
		{
			glm::vec3 pos;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color;
			glm::vec4 tangent;
		};

		// Single vertex buffer for all primitives
		struct
		{
			std::unique_ptr<Buffer> buffer;
		} vertices;

		// Single index buffer for all primitives
		struct 
		{
			int count;
			std::unique_ptr<Buffer> buffer;
		} indices;

		// The following structures roughly represent the glTF scene structure
		// To keep things simple, they only contain those properties that are required for this sample
		struct Node;

		// A primitive contains the data for a single draw call
		struct Primitive 
		{
			uint32_t firstIndex;
			uint32_t indexCount;
			int32_t materialIndex;
		};

		// Contains the node's (optional) geometry and can be made up of an arbitrary number of primitives
		struct Mesh 
		{
			std::vector<Primitive> primitives;
		};


		// A node represents an object in the glTF scene graph
		struct Node {
			Node* parent;
			std::vector<Node> children;
			Mesh mesh;
			glm::mat4 matrix;
			std::string name;
			bool visible = true;
		};

		// A glTF material stores information in e.g. the exture that is attached to it and colors
		struct Material {
			glm::vec4 baseColorFactor = glm::vec4(1.0f);
			uint32_t baseColorTextureIndex;
			uint32_t normalTextureIndex;
			std::string alphaMode = "OPAQUE";
			float alphaCutOff;
			bool doubleSided = false;
			VkDescriptorSet descriptorSet;
			VkPipeline pipeline;
		};

		// Contains the texture for a single glTF image
		// Images may be reused by texture objects and are as such separted
		struct Image 
		{
			std::string path;
			std::unique_ptr<Texture> texture;
		};

		// A glTF texture stores a reference to the image and a sampler
		// In this sample, we are only interested in the image
		struct GLTFTexture 
		{
			int32_t imageIndex;
		};

		/*
			Model data
		*/
		std::vector<Image> images;
		std::vector<GLTFTexture> textures;
		std::vector<Material> materials;
		std::vector<Node> nodes;

		std::string path;

		GLTF(TendouDevice& d);

		~GLTF();
		VkDescriptorImageInfo GetTextureDescriptor(const size_t index);
		void LoadImages(tinygltf::Model& input);
		void LoadTextures(tinygltf::Model& input);
		void LoadMaterials(tinygltf::Model& input);
		void LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, 
			GLTF::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<GLTF::Vertex>& vertexBuffer);
		void DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, GLTF::Node node);
		void Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);
	};

	class GLTFScene : public Scene
	{
	public:
		GLTF glTFScene;

		struct ShaderData 
		{
			std::unique_ptr<Buffer> buffer;
			struct Values 
			{
				glm::mat4 projection;
				glm::mat4 view;
				glm::vec4 lightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
				glm::vec4 viewPos;
			} values;
		} shaderData;

		VkPipelineLayout pipelineLayout;
		VkDescriptorSet descriptorSet;

		struct DescriptorSetLayouts 
		{
			VkDescriptorSetLayout matrices;
			VkDescriptorSetLayout textures;
		} descriptorSetLayouts;


		GLTFScene(Window& window, TendouDevice& device);
		~GLTFScene() override;

		int Init() override;
		int PreUpdate() override;
		int Update() override;
		int PostUpdate() override;

		int Render(VkCommandBuffer buf, FrameInfo& f) override;

	private:
		void LoadGLTFFile(std::string path);
		void SetupDescriptors();
		void PreparePipelines();
		void PrepareUniformBuffers();
		void UpdateUniformBuffers();

		void ShowCheckbox(Tendou::GLTF::Node& node);
	};
}

#endif