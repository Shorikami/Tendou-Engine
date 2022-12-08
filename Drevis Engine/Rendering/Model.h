#ifndef MODEL_H
#define MODEL_H

#include "../Vulkan/DrevisDevice.h"
#include "Buffer.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

#include <memory>
#include <vector>

namespace Drevis
{
	class Model
	{
	public:
		enum class Type
		{
			OBJ = 0,
			GLTF
		};

		struct Vertex
		{
			glm::vec3 position;
			glm::vec3 normal;
			glm::vec2 uv;
			glm::vec3 color;

			static std::vector<VkVertexInputBindingDescription> GetBindingDescriptions();
			static std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions();

			bool operator==(const Vertex& other) const
			{
				return position == other.position &&
					color == other.color &&
					normal == other.normal &&
					uv == other.uv;
			}
		};

		template <typename T>
		struct Builder
		{
			std::vector<T> vertices{};
			std::vector<uint32_t> indices{};

			void LoadOBJ(const std::string& filePath, bool flipY, const std::string& mtlPath = std::string());
			void LoadGLTF(const std::string& filePath, bool flipY);
		};

		template <typename T>
		Model(DrevisDevice& device, const Model::Builder<T>& verts);
		~Model();

		Model(const Model&) = delete;
		Model& operator=(const Model&) = delete;

		static std::unique_ptr<Model> CreateModelFromFile(DrevisDevice& device, 
			const std::string& filePath, const std::string& mtlPath = std::string(), bool flipY = false);

		void Bind(VkCommandBuffer commandBuffer);
		void Draw(VkCommandBuffer commandBuffer);

	private:
		void CreateVertexBuffers(const std::vector<Vertex>& verts);
		void CreateIndexBuffers(const std::vector<uint32_t>& indices);

		DrevisDevice& device_;

		std::unique_ptr<Buffer> vertexBuffer;
		uint32_t vertexCount;

		bool hasIndexBuffer = false;
		std::unique_ptr<Buffer> indexBuffer;
		uint32_t indexCount;
	};
}

#endif