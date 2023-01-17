#ifndef PIPELINE_HPP
#define PIPELINE_HPP

#include "../Vulkan/TendouDevice.h"

#include <string>
#include <vector>

namespace Tendou
{
	struct PipelineConfigInfo 
	{
		PipelineConfigInfo() = default;
		PipelineConfigInfo(const PipelineConfigInfo&) = delete;
		PipelineConfigInfo& operator=(const PipelineConfigInfo&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo;
		VkPipelineInputAssemblyStateCreateInfo inputInfo;
		VkPipelineRasterizationStateCreateInfo rasterizationInfo;
		VkPipelineMultisampleStateCreateInfo multisampleInfo;
		VkPipelineColorBlendAttachmentState colorBlendAttachment;
		VkPipelineColorBlendStateCreateInfo colorBlendInfo;
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
		
		std::vector<VkDynamicState> dynamicStateEnables;
		VkPipelineDynamicStateCreateInfo dynamicStateInfo;

		VkPipelineLayout pipelineLayout = nullptr;
		VkRenderPass renderPass = nullptr;
		uint32_t subpass = 0;
	};

	class Pipeline
	{
	public:
		Pipeline() = default;
		Pipeline(TendouDevice& device,
				const std::string& vertPath, 
				const std::string& fragPath,
				const PipelineConfigInfo& configInfo);
		~Pipeline();

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;

		void Bind(VkCommandBuffer commandBuffer);
		void Unbind(VkCommandBuffer commandBuffer);

		static void DefaultPipelineConfigInfo(PipelineConfigInfo& configInfo);
		static std::vector<char> ReadFile(const std::string& filePath);
	private:
		void CreateGraphicsPipeline(const std::string& vertPath, 
			const std::string& fragPath,
			const PipelineConfigInfo& configInfo);

		void CreateShaderModule(const std::vector<char>& source, VkShaderModule* shaderModule);

		TendouDevice& drevisDevice;
		VkPipeline graphicsPipeline;
		VkShaderModule vertShaderModule, fragShaderModule;

		bool bound = false;
	};
}

#endif