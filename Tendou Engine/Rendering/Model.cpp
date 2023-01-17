#include "Model.h"

#include "../Utilities/Hasher.hpp"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <cassert>
#include <iostream>
#include <unordered_map>

namespace std
{
	template<>
	struct hash<Tendou::Model::Vertex>
	{
		size_t operator()(Tendou::Model::Vertex const& v) const
		{
			size_t seed = 0;
			Tendou::HashCombine(seed, v.position, v.color, v.normal, v.uv);

			return seed;
		}
	};
}

namespace Tendou
{
	std::vector<VkVertexInputBindingDescription> Model::Vertex::GetBindingDescriptions()
	{
		std::vector< VkVertexInputBindingDescription> bindingDesc(1);
		bindingDesc[0].binding = 0;
		bindingDesc[0].stride = sizeof(Vertex);
		bindingDesc[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDesc;
	}

	std::vector<VkVertexInputAttributeDescription> Model::Vertex::GetAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attDesc{};

		attDesc.push_back({ 0, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, position) });
		attDesc.push_back({ 1, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, normal) });
		attDesc.push_back({ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, uv) });
		attDesc.push_back({ 3, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Vertex, color) });

		return attDesc;
	}

	template <typename T>
	Model::Model(TendouDevice& device, const Model::Builder<T>& builder)
		: device_(device)
	{
		CreateVertexBuffers(builder.vertices);
		CreateIndexBuffers(builder.indices);
	}

	std::unique_ptr<Model> Model::CreateModelFromFile(TendouDevice& device, Type type,
		const std::string& filePath, const std::string& mtlPath, bool flipY)
	{
		std::unique_ptr<Model> res(nullptr);

		switch (type)
		{
		//case Type::GLTF:
		//{
		//	Builder<Model::Vertex> b{};
		//	b.LoadGLTF(filePath, flipY);
		//
		//	std::cout << "Vertex count: " << b.vertices.size() << std::endl;
		//	res = std::make_unique<Model>(device, b);
		//	break;
		//}
		default:
			Builder<Model::Vertex> builder{};
			builder.LoadOBJ(filePath, flipY, mtlPath);

			std::cout << "Vertex count: " << builder.vertices.size() << std::endl;
			res = std::make_unique<Model>(device, builder);
			break;
		}

		return res;
	}

	Model::~Model()
	{
	}

	void Model::CreateVertexBuffers(const std::vector<Vertex>& verts)
	{
		vertexCount = static_cast<uint32_t>(verts.size());
		assert(vertexCount >= 3 && "Vertex count must be at least 3!");

		VkDeviceSize bufSize = sizeof(verts[0]) * vertexCount;
		uint32_t vertexSize = sizeof(verts[0]);

		Buffer stagingBuffer
		{
			device_,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)verts.data());

		vertexBuffer = std::make_unique<Buffer>(
			device_,
			vertexSize,
			vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device_.CopyBuffer(stagingBuffer.GetBuffer(), vertexBuffer->GetBuffer(), bufSize);
	}

	void Model::CreateIndexBuffers(const std::vector<uint32_t>& indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;

		if (!hasIndexBuffer)
		{
			return;
		}

		VkDeviceSize bufSize = sizeof(indices[0]) * indexCount;
		uint32_t indexSize = sizeof(indices[0]);

		Buffer stagingBuffer
		{
			device_,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};

		stagingBuffer.Map();
		stagingBuffer.WriteToBuffer((void*)indices.data());

		indexBuffer = std::make_unique<Buffer>(
			device_,
			indexSize,
			indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device_.CopyBuffer(stagingBuffer.GetBuffer(), indexBuffer->GetBuffer(), bufSize);
	}

	void Model::Bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { vertexBuffer->GetBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

		if (hasIndexBuffer)
		{
			vkCmdBindIndexBuffer(commandBuffer, indexBuffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);
		}
	}

	void Model::Draw(VkCommandBuffer commandBuffer)
	{
		if (hasIndexBuffer)
		{
			vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
		}
		else
		{
			vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
		}
	}

	template <typename T>
	void Model::Builder<T>::LoadOBJ(const std::string& f, bool flipY, const std::string& m)
	{
		int multiplier = !flipY ? 1 : -1;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, f.c_str(), m.c_str()))
		{
			throw std::runtime_error(warn + err);
		}

		vertices.clear();
		indices.clear();

		std::unordered_map<T, uint32_t> uniqueVerts{};

		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex v{};

				if (index.vertex_index >= 0)
				{
					v.position =
					{
						attrib.vertices[3 * index.vertex_index + 0],
						multiplier * attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2],
					};

					v.color =
					{
						attrib.colors[3 * index.vertex_index + 0],
						attrib.colors[3 * index.vertex_index + 1],
						attrib.colors[3 * index.vertex_index + 2],
					};

				}

				if (index.normal_index >= 0)
				{
					hasNormals = true;
					v.normal =
					{
						attrib.normals[3 * index.normal_index + 0],
						multiplier * attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2],
					};
				}

				if (index.texcoord_index >= 0)
				{
					v.uv =
					{
						attrib.texcoords[2 * index.texcoord_index + 0],
						1 - attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}

				if (uniqueVerts.count(v) == 0)
				{
					uniqueVerts[v] = static_cast<uint32_t>(vertices.size());
					vertices.push_back(v);
				}
				indices.push_back(uniqueVerts[v]);
			}
		}

		if (!hasNormals)
		{
			std::vector<glm::vec3> vvv;
			std::vector<glm::vec3> temp(vertices.size());

			// TODO: Manual normal generation
			uint32_t idx = 0;

			for (; idx < indices.size();)
			{
				GLuint a = indices.at(idx++);
				GLuint b = indices.at(idx++);
				GLuint c = indices.at(idx++);

				glm::vec3 vA = vertices[a].position;
				glm::vec3 vB = vertices[b].position;
				glm::vec3 vC = vertices[c].position;

				glm::vec3 E1 = vB - vA;
				glm::vec3 E2 = vC - vA;

				glm::vec3 N = glm::normalize(glm::cross(E1, E2));

				if (N.x < 0.0001f && N.x > -0.0001f)
				{
					N.x = 0.0f;
				}
				if (N.y < 0.0001f && N.y > -0.0001f)
				{
					N.y = 0.0f;
				}
				if (N.z < 0.0001f && N.z > -0.0001f)
				{
					N.z = 0.0f;
				}

				vvv.push_back(N);
			}

			for (int idx = 0; idx < vertices.size(); ++idx)
			{
				glm::vec3 vNormal(0.0f);

				int bb = 0;
				std::vector<glm::vec3> dup;

				for (int kk = 0; bb < indices.size(); ++kk)
				{
					GLuint a = indices.at(bb++);
					GLuint b = indices.at(bb++);
					GLuint c = indices.at(bb++);

					if (a == idx || b == idx || c == idx)
					{
						bool isDup = false;

						for (int k = 0; k < dup.size(); ++k)
						{
							if (vvv[kk] == dup[k])
							{
								isDup = true;
							}
						}

						if (!isDup)
						{
							dup.push_back(vvv[kk]);
							vNormal += vvv[kk];
						}
					}
				}

				vertices[idx].normal = glm::normalize(vNormal);
			}
		}
	}

	//template <typename T>
	//void Model::Builder<T>::LoadGLTF(const std::string& f, bool flipY)
	//{
	//	tinygltf::Model model;
	//	tinygltf::TinyGLTF loader;
	//	std::string err;
	//	std::string warn;
	//
	//	bool res = loader.LoadASCIIFromFile(&model, &err, &warn, f);
	//
	//	if (!warn.empty()) 
	//	{
	//		std::cout << "WARN: " << warn << std::endl;
	//	}
	//
	//	if (!err.empty()) 
	//	{
	//		std::cout << "ERR: " << err << std::endl;
	//	}
	//
	//	if (!res)
	//		std::cout << "Failed to load glTF: " << f << std::endl;
	//	else
	//		std::cout << "Loaded glTF: " << f << std::endl;
	//
	//	const tinygltf::Scene& scene = model.scenes[model.defaultScene];
	//
	//	for (size_t i = 0; i < scene.nodes.size(); ++i)
	//	{
	//		assert((scene.nodes[i] >= 0) && (scene.nodes[i] < model.nodes.size()));
	//	}
	//}
}
