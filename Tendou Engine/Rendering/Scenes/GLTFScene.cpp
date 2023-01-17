#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define TINYGLTF_NO_STB_IMAGE_WRITE
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_EXTERNAL_IMAGE
#ifdef VK_USE_PLATFORM_ANDROID_KHR
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "GLTFScene.h"

#include <iostream>

namespace Tendou
{
	GLTF::GLTF(TendouDevice& device)
		: device_(device)
	{
	}

	GLTF::~GLTF()
	{
		//// Release all Vulkan resources allocated for the model
		//vkDestroyBuffer(device_.Device(), vertices.buffer, nullptr);
		//vkFreeMemory(device_.Device(), vertices.memory, nullptr);
		//vkDestroyBuffer(device_.Device(), indices.buffer, nullptr);
		//vkFreeMemory(device_.Device(), indices.memory, nullptr);

		//for (Image image : images) 
		//{
		//	vkDestroyImageView(device_.Device(), image.texture.TextureImageView(), nullptr);
		//	vkDestroyImage(device_.Device(), image.texture.TextureImage(), nullptr);
		//	vkDestroySampler(device_.Device(), image.texture.TextureSampler(), nullptr);
		//	vkFreeMemory(device_.Device(), image.texture.TextureMemory(), nullptr);
		//}
		
		for (Material material : materials) 
		{
			vkDestroyPipeline(device_.Device(), material.pipeline, nullptr);
		}
	}

	VkDescriptorImageInfo GLTF::GetTextureDescriptor(const size_t index)
	{
		return images[index].texture.get()->DescriptorInfo();
	}

	void GLTF::LoadImages(tinygltf::Model& input)
	{
		// POI: The textures for the glTF file used in this sample are stored as external ktx files, so we can directly load them from disk without the need for conversion
		images.resize(input.images.size());

		for (size_t i = 0; i < input.images.size(); ++i) 
		{
			tinygltf::Image& glTFImage = input.images[i];
			//images[i].texture.loadFromFile(path + "/" + glTFImage.uri, VK_FORMAT_R8G8B8A8_UNORM, vulkanDevice, copyQueue);
			images[i].texture = std::make_unique<Texture>(device_, path + "/" + glTFImage.uri);
			images[i].path = glTFImage.uri;
		}

	}

	void GLTF::LoadTextures(tinygltf::Model& input)
	{
		textures.resize(input.textures.size());

		for (size_t i = 0; i < input.textures.size(); ++i) 
		{
			textures[i].imageIndex = input.textures[i].source;
		}
	}

	void GLTF::LoadMaterials(tinygltf::Model& input)
	{
		materials.resize(input.materials.size());

		for (size_t i = 0; i < input.materials.size(); ++i) 
		{
			// We only read the most basic properties required for our sample
			tinygltf::Material glTFMaterial = input.materials[i];

			// Get the base color factor
			if (glTFMaterial.values.find("baseColorFactor") != glTFMaterial.values.end()) 
			{
				materials[i].baseColorFactor = glm::make_vec4(glTFMaterial.values["baseColorFactor"].ColorFactor().data());
			}

			// Get base color texture index
			if (glTFMaterial.values.find("baseColorTexture") != glTFMaterial.values.end()) 
			{
				materials[i].baseColorTextureIndex = glTFMaterial.values["baseColorTexture"].TextureIndex();
			}

			// Get the normal map texture index
			if (glTFMaterial.additionalValues.find("normalTexture") != glTFMaterial.additionalValues.end()) 
			{
				materials[i].normalTextureIndex = glTFMaterial.additionalValues["normalTexture"].TextureIndex();
			}

			// Get some additonal material parameters that are used in this sample
			materials[i].alphaMode = glTFMaterial.alphaMode;
			materials[i].alphaCutOff = static_cast<float>(glTFMaterial.alphaCutoff);
			materials[i].doubleSided = glTFMaterial.doubleSided;
		}
	}

	void GLTF::LoadNode(const tinygltf::Node& inputNode, const tinygltf::Model& input, 
		GLTF::Node* parent, std::vector<uint32_t>& indexBuffer, std::vector<GLTF::Vertex>& vertexBuffer)
	{
		GLTF::Node node{};
		node.name = inputNode.name;

		// Get the local node matrix
		// It's either made up from translation, rotation, scale or a 4x4 matrix
		node.matrix = glm::mat4(1.0f);
		if (inputNode.translation.size() == 3) 
		{
			node.matrix = glm::translate(node.matrix, glm::vec3(glm::make_vec3(inputNode.translation.data())));
		}
		if (inputNode.rotation.size() == 4) 
		{
			glm::quat q = glm::make_quat(inputNode.rotation.data());
			node.matrix *= glm::mat4(q);
		}
		if (inputNode.scale.size() == 3) 
		{
			node.matrix = glm::scale(node.matrix, glm::vec3(glm::make_vec3(inputNode.scale.data())));
		}
		if (inputNode.matrix.size() == 16) 
		{
			node.matrix = glm::make_mat4x4(inputNode.matrix.data());
		};

		// Load node's children
		if (inputNode.children.size() > 0) {
			for (size_t i = 0; i < inputNode.children.size(); i++) {
				LoadNode(input.nodes[inputNode.children[i]], input, &node, indexBuffer, vertexBuffer);
			}
		}

		// If the node contains mesh data, we load vertices and indices from the the buffers
		// In glTF this is done via accessors and buffer views
		if (inputNode.mesh > -1) 
		{
			const tinygltf::Mesh mesh = input.meshes[inputNode.mesh];
			// Iterate through all primitives of this node's mesh
			for (size_t i = 0; i < mesh.primitives.size(); ++i) 
			{
				const tinygltf::Primitive& glTFPrimitive = mesh.primitives[i];
				uint32_t firstIndex = static_cast<uint32_t>(indexBuffer.size());
				uint32_t vertexStart = static_cast<uint32_t>(vertexBuffer.size());
				uint32_t indexCount = 0;
				// Vertices
				{
					const float* positionBuffer = nullptr;
					const float* normalsBuffer = nullptr;
					const float* texCoordsBuffer = nullptr;
					const float* tangentsBuffer = nullptr;
					size_t vertexCount = 0;

					// Get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("POSITION") != glTFPrimitive.attributes.end()) 
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("POSITION")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						positionBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
						vertexCount = accessor.count;
					}
					// Get buffer data for vertex normals
					if (glTFPrimitive.attributes.find("NORMAL") != glTFPrimitive.attributes.end()) 
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("NORMAL")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						normalsBuffer = reinterpret_cast<const float*>
							(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// Get buffer data for vertex texture coordinates
					// glTF supports multiple sets, we only load the first one
					if (glTFPrimitive.attributes.find("TEXCOORD_0") != glTFPrimitive.attributes.end()) 
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TEXCOORD_0")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						texCoordsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// POI: This sample uses normal mapping, so we also need to load the tangents from the glTF file
					if (glTFPrimitive.attributes.find("TANGENT") != glTFPrimitive.attributes.end()) 
					{
						const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.attributes.find("TANGENT")->second];
						const tinygltf::BufferView& view = input.bufferViews[accessor.bufferView];
						tangentsBuffer = reinterpret_cast<const float*>(&(input.buffers[view.buffer].data[accessor.byteOffset + view.byteOffset]));
					}

					// Append data to model's vertex buffer
					for (size_t v = 0; v < vertexCount; ++v) 
					{
						Vertex vert{};
						vert.pos = glm::vec4(glm::make_vec3(&positionBuffer[v * 3]), 1.0f);
						vert.pos.y *= -1.0f;

						vert.normal = glm::normalize(glm::vec3(normalsBuffer ? glm::make_vec3(&normalsBuffer[v * 3]) : glm::vec3(0.0f)));
						vert.normal.y *= -1.0f;
						
						vert.uv = texCoordsBuffer ? glm::make_vec2(&texCoordsBuffer[v * 2]) : glm::vec3(0.0f);

						vert.color = glm::vec3(1.0f);
						vert.tangent = tangentsBuffer ? glm::make_vec4(&tangentsBuffer[v * 4]) : glm::vec4(0.0f);
						vertexBuffer.push_back(vert);
					}
				}
				
				// Indices
				{
					const tinygltf::Accessor& accessor = input.accessors[glTFPrimitive.indices];
					const tinygltf::BufferView& bufferView = input.bufferViews[accessor.bufferView];
					const tinygltf::Buffer& buffer = input.buffers[bufferView.buffer];

					indexCount += static_cast<uint32_t>(accessor.count);

					// glTF supports different component types of indices
					switch (accessor.componentType) {
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: 
					{
						uint32_t* buf = new uint32_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint32_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: 
					{
						uint16_t* buf = new uint16_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint16_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: 
					{
						uint8_t* buf = new uint8_t[accessor.count];
						memcpy(buf, &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(uint8_t));
						for (size_t index = 0; index < accessor.count; index++) {
							indexBuffer.push_back(buf[index] + vertexStart);
						}
						break;
					}
					default:
						std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
						return;
					}
				}
				Primitive primitive{};
				primitive.firstIndex = firstIndex;
				primitive.indexCount = indexCount;
				primitive.materialIndex = glTFPrimitive.material;
				node.mesh.primitives.push_back(primitive);
			}
		}

		if (parent) 
		{
			parent->children.push_back(node);
		}

		else 
		{
			nodes.push_back(node);
		}

	}

	void GLTF::DrawNode(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, GLTF::Node node)
	{
		if (!node.visible) 
		{
			return;
		}

		if (node.mesh.primitives.size() > 0) 
		{
			// Pass the node's matrix via push constanst
			// Traverse the node hierarchy to the top-most parent to get the final matrix of the current node
			glm::mat4 nodeMatrix = node.matrix;
			GLTF::Node* currentParent = node.parent;
			while (currentParent) {
				nodeMatrix = currentParent->matrix * nodeMatrix;
				currentParent = currentParent->parent;
			}
			// Pass the final matrix to the vertex shader using push constants
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &nodeMatrix);
			for (GLTF::Primitive& primitive : node.mesh.primitives) 
			{
				if (primitive.indexCount > 0) 
				{
					GLTF::Material& material = materials[primitive.materialIndex];
					// POI: Bind the pipeline for the node's material
					vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, material.pipeline);
					vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 1, 1, &material.descriptorSet, 0, nullptr);
					vkCmdDrawIndexed(commandBuffer, primitive.indexCount, 1, primitive.firstIndex, 0, 0);
				}
			}
		}

		for (auto& child : node.children)
		{
			DrawNode(commandBuffer, pipelineLayout, child);
		}

	}

	void GLTF::Draw(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
	{
		// All vertices and indices are stored in single buffers, so we only need to bind once
		VkBuffer buffers[] = { vertices.buffer->GetBuffer() };
		VkDeviceSize offsets[1] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indices.buffer->GetBuffer(), 0, VK_INDEX_TYPE_UINT32);

		// Render all nodes at top-level
		for (auto& node : nodes) 
		{
			DrawNode(commandBuffer, pipelineLayout, node);
		}

	}

	GLTFScene::GLTFScene(Window& window, TendouDevice& device)
		: Scene(window, device)
		, glTFScene(device)
	{
		//LoadGLTFFile("Materials/Models/GLTF/Sponza/glTF/Sponza.gltf");
		LoadGLTFFile("Materials/Models/GLTF/BA_miyu/scene.gltf");
	}

	GLTFScene::~GLTFScene()
	{

	}

	int GLTFScene::Init()
	{
		PrepareUniformBuffers();
		SetupDescriptors();
		PreparePipelines();

		return 0;
	}

	void GLTFScene::ShowCheckbox(GLTF::Node& node)
	{
		ImGui::Checkbox(node.name.c_str(), &node.visible);

		for (auto& child : node.children)
		{
			ShowCheckbox(child);
		}
	}

	int GLTFScene::PreUpdate()
	{
		if (ImGui::BeginMenu("Node Visibility"))
		{
			ImGui::BeginChild("#nodelist", ImVec2(200.0f, 340.0f), false);
			for (auto& node : glTFScene.nodes)
			{
				ShowCheckbox(node);
			}
			ImGui::EndChild();

			ImGui::EndMenu();
		}
		return 0;
	}

	int GLTFScene::Update()
	{
		UpdateUniformBuffers();
		return 0;
	}

	int GLTFScene::PostUpdate()
	{
		return 0;
	}

	int GLTFScene::Render(VkCommandBuffer buf, FrameInfo& f)
	{
		// Render the actual scene (swapchain)
		BeginSwapChainRenderPass(buf);
		vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);
		glTFScene.Draw(buf, pipelineLayout);

		return 0;
	}

	void GLTFScene::LoadGLTFFile(std::string path)
	{
		tinygltf::Model glTFInput;
		tinygltf::TinyGLTF gltfContext;
		std::string error, warning;

		bool fileLoaded = gltfContext.LoadASCIIFromFile(&glTFInput, &error, &warning, path);

		//// Pass some Vulkan resources required for setup and rendering to the glTF model loading class
		//glTFScene.vulkanDevice = vulkanDevice;
		//glTFScene.copyQueue = queue;

		size_t pos = path.find_last_of('/');
		glTFScene.path = path.substr(0, pos);

		std::vector<uint32_t> indexBuffer;
		std::vector<GLTF::Vertex> vertexBuffer;

		if (fileLoaded) {
			glTFScene.LoadTextures(glTFInput);
			glTFScene.LoadImages(glTFInput);
			glTFScene.LoadMaterials(glTFInput);
			const tinygltf::Scene& scene = glTFInput.scenes[0];

			for (size_t i = 0; i < scene.nodes.size(); ++i) 
			{
				const tinygltf::Node node = glTFInput.nodes[scene.nodes[i]];
				glTFScene.LoadNode(node, glTFInput, nullptr, indexBuffer, vertexBuffer);
			}
		}

		else 
		{
			std::cout << "Uh oh!" << std::endl;
			return;
		}

		// Create and upload vertex and index buffer
		// We will be using one single vertex buffer and one single index buffer for the whole glTF scene
		// Primitives (of the glTF model) will then index into these using index offsets

		size_t vertexBufferSize = vertexBuffer.size() * sizeof(GLTF::Vertex);
		size_t indexBufferSize = indexBuffer.size() * sizeof(uint32_t);
		glTFScene.indices.count = static_cast<uint32_t>(indexBuffer.size());

		//struct StagingBuffer 
		//{
		//	VkBuffer buffer;
		//	VkDeviceMemory memory;
		//} vertexStaging, indexStaging;

		Buffer vertexStaging
		{
			device,
			sizeof(GLTF::Vertex),
			static_cast<uint32_t>(vertexBuffer.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};
		
		vertexStaging.Map();
		vertexStaging.WriteToBuffer((void*)vertexBuffer.data());

		Buffer indexStaging
		{
			device,
			sizeof(uint32_t),
			static_cast<uint32_t>(indexBuffer.size()),
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		};
		
		indexStaging.Map();
		indexStaging.WriteToBuffer((void*)indexBuffer.data());
		
		//indexBuffer = std::make_unique<Buffer>(
		//	device_,
		//	indexSize,
		//	indexCount,
		//	VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
		//	VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
		//
		//device_.CopyBuffer(stagingBuffer.GetBuffer(), indexBuffer->GetBuffer(), bufSize);

		//// Create host visible staging buffers (source)
		//VK_CHECK_RESULT(vulkanDevice->createBuffer(
		//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	vertexBufferSize,
		//	&vertexStaging.buffer,
		//	&vertexStaging.memory,
		//	vertexBuffer.data()));
		//// Index data
		//VK_CHECK_RESULT(vulkanDevice->createBuffer(
		//	VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		//	VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		//	indexBufferSize,
		//	&indexStaging.buffer,
		//	&indexStaging.memory,
		//	indexBuffer.data()));

		// Create device local buffers (target)

		glTFScene.vertices.buffer = std::make_unique<Buffer>(
			device,
			sizeof(GLTF::Vertex),
			static_cast<uint32_t>(vertexBuffer.size()),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		glTFScene.indices.buffer = std::make_unique<Buffer>(
			device,
			sizeof(uint32_t),
			static_cast<uint32_t>(indexBuffer.size()),
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device.CopyBuffer(vertexStaging.GetBuffer(), glTFScene.vertices.buffer->GetBuffer(), vertexBufferSize);
		device.CopyBuffer(indexStaging.GetBuffer(), glTFScene.indices.buffer->GetBuffer(), indexBufferSize);

		//// Copy data from staging buffers (host) do device local buffer (gpu)
		//VkCommandBuffer copyCmd = vulkanDevice->createCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY, true);
		//VkBufferCopy copyRegion = {};
		//
		//copyRegion.size = vertexBufferSize;
		//vkCmdCopyBuffer(
		//	copyCmd,
		//	vertexStaging.buffer,
		//	glTFScene.vertices.buffer,
		//	1,
		//	&copyRegion);
		//
		//copyRegion.size = indexBufferSize;
		//vkCmdCopyBuffer(
		//	copyCmd,
		//	indexStaging.buffer,
		//	glTFScene.indices.buffer,
		//	1,
		//	&copyRegion);
		//
		//vulkanDevice->flushCommandBuffer(copyCmd, queue, true);

		//// Free staging resources
		//vkDestroyBuffer(device.Device(), vertexStaging.GetBuffer(), nullptr);
		//vkFreeMemory(device, vertexStaging.GetBuffer(), nullptr);
		//vkDestroyBuffer(device, indexStaging.buffer, nullptr);
		//vkFreeMemory(device, indexStaging.memory, nullptr);

	}
	
	void GLTFScene::SetupDescriptors()
	{
		/*
			This sample uses separate descriptor sets (and layouts) for the matrices and materials (textures)
		*/
		const uint32_t maxSetCount = static_cast<uint32_t>(glTFScene.images.size()) + 1;
		const uint32_t maxCount = maxSetCount > glTFScene.materials.size() * 2 ? maxSetCount : glTFScene.materials.size() * 4;

		// One ubo to pass dynamic data to the shader
		// Two combined image samplers per material as each material uses color and normal maps
		globalPool = DescriptorPool::Builder(device)
			.SetMaxSets(maxCount)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1)
			.AddPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(glTFScene.materials.size()) * 2)
			.Build();

		//std::vector<VkDescriptorPoolSize> poolSizes = {
		//	vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1),
		//	vks::initializers::descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(glTFScene.materials.size()) * 2),
		//};
		// One set for matrices and one per model image/texture
		//const uint32_t maxSetCount = static_cast<uint32_t>(glTFScene.images.size()) + 1;
		//VkDescriptorPoolCreateInfo descriptorPoolInfo = vks::initializers::descriptorPoolCreateInfo(poolSizes, maxSetCount);
		//VK_CHECK_RESULT(vkCreateDescriptorPool(device, &descriptorPoolInfo, nullptr, &descriptorPool));

		// Descriptor set layout for passing matrices
		VkDescriptorSetLayoutBinding setLayoutBinding{};
		setLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		setLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		setLayoutBinding.binding = 0;
		setLayoutBinding.descriptorCount = 1;

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
		descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutCI.pBindings = &setLayoutBinding;
		descriptorSetLayoutCI.bindingCount = 1;

		vkCreateDescriptorSetLayout(device.Device(), &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.matrices);

		// Descriptor set layout for passing material textures
		
		// Color map
		VkDescriptorSetLayoutBinding color{};
		color.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		color.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		color.binding = 0;
		color.descriptorCount = 1;

		// Normal map
		VkDescriptorSetLayoutBinding normal{};
		normal.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		normal.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		normal.binding = 1;
		normal.descriptorCount = 1;

		std::vector<VkDescriptorSetLayoutBinding> materialBindings =
		{
			color,
			normal
		};

		descriptorSetLayoutCI.pBindings = materialBindings.data();
		descriptorSetLayoutCI.bindingCount = 2;
		vkCreateDescriptorSetLayout(device.Device(), &descriptorSetLayoutCI, nullptr, &descriptorSetLayouts.textures);

		// Pipeline layout using both descriptor sets (set 0 = matrices, set 1 = material)
		std::array<VkDescriptorSetLayout, 2> setLayouts = { descriptorSetLayouts.matrices, descriptorSetLayouts.textures };

		VkPipelineLayoutCreateInfo pipelineLayoutCI{};
		pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutCI.setLayoutCount = static_cast<uint32_t>(setLayouts.size());
		pipelineLayoutCI.pSetLayouts = setLayouts.data();


		// We will use push constants to push the local matrices of a primitive to the vertex shader
		VkPushConstantRange pushConstantRange{};
		pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		pushConstantRange.offset = 0;
		pushConstantRange.size = sizeof(glm::mat4);


		// Push constant ranges are part of the pipeline layout
		pipelineLayoutCI.pushConstantRangeCount = 1;
		pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
		vkCreatePipelineLayout(device.Device(), &pipelineLayoutCI, nullptr, &pipelineLayout);

		// Descriptor set for scene matrices
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = globalPool->GetDescriptorPool();
		allocInfo.pSetLayouts = &descriptorSetLayouts.matrices;
		allocInfo.descriptorSetCount = 1;


		auto bufInfo = shaderData.buffer->DescriptorInfo();

		vkAllocateDescriptorSets(device.Device(), &allocInfo, &descriptorSet);
		VkWriteDescriptorSet writeDescriptorSet{};
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		writeDescriptorSet.dstSet = descriptorSet;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.pBufferInfo = &bufInfo;
		writeDescriptorSet.descriptorCount = 1;

		vkUpdateDescriptorSets(device.Device(), 1, &writeDescriptorSet, 0, nullptr);

		// Descriptor sets for materials
		for (auto& material : glTFScene.materials) 
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = globalPool->GetDescriptorPool();
			allocInfo.pSetLayouts = &descriptorSetLayouts.textures;
			allocInfo.descriptorSetCount = 1;
			
			vkAllocateDescriptorSets(device.Device(), &allocInfo, &material.descriptorSet);

			VkDescriptorImageInfo colorMap = glTFScene.GetTextureDescriptor(material.baseColorTextureIndex);
			VkDescriptorImageInfo normalMap = glTFScene.GetTextureDescriptor(material.normalTextureIndex);

			VkWriteDescriptorSet colorSet{};
			colorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			colorSet.dstSet = material.descriptorSet;
			colorSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			colorSet.dstBinding = 0;
			colorSet.pImageInfo = &colorMap;
			colorSet.descriptorCount = 1;

			VkWriteDescriptorSet normalSet{};
			normalSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			normalSet.dstSet = material.descriptorSet;
			normalSet.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			normalSet.dstBinding = 1;
			normalSet.pImageInfo = &normalMap;
			normalSet.descriptorCount = 1;

			std::vector<VkWriteDescriptorSet> writeDescriptorSets = 
			{
				colorSet,
				normalSet
			};
			vkUpdateDescriptorSets(device.Device(), static_cast<uint32_t>(writeDescriptorSets.size()), 
				writeDescriptorSets.data(), 0, nullptr);
		}
	}

	void GLTFScene::PreparePipelines()
	{
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCI{};
		inputAssemblyStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssemblyStateCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssemblyStateCI.flags = 0;
		inputAssemblyStateCI.primitiveRestartEnable = VK_FALSE;

		VkPipelineRasterizationStateCreateInfo rasterizationStateCI{};
		rasterizationStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizationStateCI.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizationStateCI.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizationStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizationStateCI.flags = 0;
		rasterizationStateCI.depthClampEnable = VK_FALSE;
		rasterizationStateCI.lineWidth = 1.0f;

		VkPipelineColorBlendAttachmentState blendAttachmentStateCI{};
		blendAttachmentStateCI.colorWriteMask = 0xf;
		blendAttachmentStateCI.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlendStateCI{};
		colorBlendStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendStateCI.attachmentCount = 1;
		colorBlendStateCI.pAttachments = &blendAttachmentStateCI;

		VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
		depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencilStateCI.depthTestEnable = VK_TRUE;
		depthStencilStateCI.depthWriteEnable = VK_TRUE;
		depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		depthStencilStateCI.back.compareOp = VK_COMPARE_OP_ALWAYS;

		VkPipelineViewportStateCreateInfo viewportStateCI{};
		viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportStateCI.viewportCount = 1;
		viewportStateCI.scissorCount = 1;
		viewportStateCI.flags = 0;

		VkPipelineMultisampleStateCreateInfo multisampleStateCI{};
		multisampleStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampleStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampleStateCI.flags = 0;

		const std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };

		VkPipelineDynamicStateCreateInfo dynamicStateCI{};
		dynamicStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicStateCI.pDynamicStates = dynamicStateEnables.data();
		dynamicStateCI.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
		dynamicStateCI.flags = 0;

		
		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages;

		//VkVertexInputBindingDescription vInputBindDescription{};
		//vInputBindDescription.binding = 0;
		//vInputBindDescription.stride = sizeof(GLTF::Vertex);
		//vInputBindDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;


		const std::vector<VkVertexInputBindingDescription> vertexInputBindings = 
		{
			VkVertexInputBindingDescription{ 0, sizeof(GLTF::Vertex), VK_VERTEX_INPUT_RATE_VERTEX }
		};

		//VkVertexInputAttributeDescription vInputAttribDescription{};
		//vInputAttribDescription.location = location;
		//vInputAttribDescription.binding = binding;
		//vInputAttribDescription.format = format;
		//vInputAttribDescription.offset = offset;


		const std::vector<VkVertexInputAttributeDescription> vertexInputAttributes = 
		{
			VkVertexInputAttributeDescription{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GLTF::Vertex, pos) },
			VkVertexInputAttributeDescription{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GLTF::Vertex, normal) },
			VkVertexInputAttributeDescription{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GLTF::Vertex, uv) },
			VkVertexInputAttributeDescription{ 3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GLTF::Vertex, color) },
			VkVertexInputAttributeDescription{ 4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(GLTF::Vertex, tangent) }
			//vks::initializers::vertexInputAttributeDescription(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFScene::Vertex, pos)),
			//vks::initializers::vertexInputAttributeDescription(0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFScene::Vertex, normal)),
			//vks::initializers::vertexInputAttributeDescription(0, 2, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFScene::Vertex, uv)),
			//vks::initializers::vertexInputAttributeDescription(0, 3, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFScene::Vertex, color)),
			//vks::initializers::vertexInputAttributeDescription(0, 4, VK_FORMAT_R32G32B32_SFLOAT, offsetof(VulkanglTFScene::Vertex, tangent)),
		};

		VkPipelineVertexInputStateCreateInfo vertexInputStateCI{};
		vertexInputStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputStateCI.vertexBindingDescriptionCount = static_cast<uint32_t>(vertexInputBindings.size());
		vertexInputStateCI.pVertexBindingDescriptions = vertexInputBindings.data();
		vertexInputStateCI.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertexInputAttributes.size());
		vertexInputStateCI.pVertexAttributeDescriptions = vertexInputAttributes.data();

		VkGraphicsPipelineCreateInfo pipelineCI{};
		pipelineCI.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCI.layout = pipelineLayout;
		pipelineCI.renderPass = GetSwapChainRenderPass();
		pipelineCI.flags = 0;
		pipelineCI.basePipelineIndex = -1;
		pipelineCI.basePipelineHandle = VK_NULL_HANDLE;


		pipelineCI.pVertexInputState = &vertexInputStateCI;
		pipelineCI.pInputAssemblyState = &inputAssemblyStateCI;
		pipelineCI.pRasterizationState = &rasterizationStateCI;
		pipelineCI.pColorBlendState = &colorBlendStateCI;
		pipelineCI.pMultisampleState = &multisampleStateCI;
		pipelineCI.pViewportState = &viewportStateCI;
		pipelineCI.pDepthStencilState = &depthStencilStateCI;
		pipelineCI.pDynamicState = &dynamicStateCI;
		pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineCI.pStages = shaderStages.data();

		VkPipelineShaderStageCreateInfo vertShader = {};
		vertShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShader.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShader.pName = "main";

		auto vertCode = Pipeline::ReadFile("Materials/Shaders/GLTF.vert.spv");

		VkShaderModule vertModule;
		VkShaderModuleCreateInfo moduleCreateInfo;
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = vertCode.size();
		moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertCode.data());
		moduleCreateInfo.flags = 0;
		vkCreateShaderModule(device.Device(), &moduleCreateInfo, NULL, &vertModule);

		vertShader.module = vertModule;

		VkPipelineShaderStageCreateInfo fragShader = {};
		fragShader.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShader.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShader.pName = "main";

		auto fragCode = Pipeline::ReadFile("Materials/Shaders/GLTF.frag.spv");

		VkShaderModule fragModule;
		moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		moduleCreateInfo.pNext = NULL;
		moduleCreateInfo.codeSize = fragCode.size();
		moduleCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragCode.data());
		moduleCreateInfo.flags = 0;
		vkCreateShaderModule(device.Device(), &moduleCreateInfo, NULL, &fragModule);

		fragShader.module = fragModule;

		shaderStages[0] = vertShader;
		shaderStages[1] = fragShader;


		//VkPipelineCache pipelineCache;
		//
		//VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
		//pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
		//vkCreatePipelineCache(device.Device(), &pipelineCacheCreateInfo, nullptr, &pipelineCache);

		// POI: Instead if using a few fixed pipelines, we create one pipeline for each material using the properties of that material
		for (auto& material : glTFScene.materials) 
		{
			struct MaterialSpecializationData 
			{
				VkBool32 alphaMask;
				float alphaMaskCutoff;
			} materialSpecializationData;

			materialSpecializationData.alphaMask = material.alphaMode == "MASK";
			materialSpecializationData.alphaMaskCutoff = material.alphaCutOff;

			// POI: Constant fragment shader material parameters will be set using specialization constants
			std::vector<VkSpecializationMapEntry> specializationMapEntries = 
			{
				VkSpecializationMapEntry{ 0, offsetof(MaterialSpecializationData, alphaMask), sizeof(MaterialSpecializationData::alphaMask) },
				VkSpecializationMapEntry{ 1, offsetof(MaterialSpecializationData, alphaMaskCutoff), sizeof(MaterialSpecializationData::alphaMaskCutoff) }
			};

			VkSpecializationInfo specializationInfo{};
			specializationInfo.mapEntryCount = static_cast<uint32_t>(specializationMapEntries.size());
			specializationInfo.pMapEntries = specializationMapEntries.data();
			specializationInfo.dataSize = sizeof(materialSpecializationData);
			specializationInfo.pData = &materialSpecializationData;


			shaderStages[1].pSpecializationInfo = &specializationInfo;

			// For double sided materials, culling will be disabled
			rasterizationStateCI.cullMode = material.doubleSided ? VK_CULL_MODE_NONE : VK_CULL_MODE_BACK_BIT;
		
			vkCreateGraphicsPipelines(device.Device(), VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &material.pipeline);
		}
		//vkDestroyPipelineCache(device.Device(), pipelineCache, nullptr);
	}

	void GLTFScene::PrepareUniformBuffers()
	{
		shaderData.buffer = std::make_unique<Buffer>(
			device,
			sizeof(shaderData.values),
			1,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
			);
		shaderData.buffer->Map();

		UpdateUniformBuffers();
	}

	void GLTFScene::UpdateUniformBuffers()
	{
		shaderData.values.projection = c.perspective();
		shaderData.values.view = c.view();
		shaderData.values.viewPos = glm::vec4(c.cameraPos, 1.0f);
		memcpy(shaderData.buffer->GetMappedMemory(), &shaderData.values, sizeof(shaderData.values));
	}
}