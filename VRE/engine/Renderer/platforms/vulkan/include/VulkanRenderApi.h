#pragma once

#include <RenderApi.h>
#include <vulkan/vulkan_raii.hpp>

namespace VRE
{
	class VulkanRenderApi : public RenderApi {
	public:
		virtual void Init() override;
		virtual void CleanUp() override;
		virtual void DrawFrame() override;
		vk::raii::Device& GetDevice() { return m_Device; }

	private:
		void CreateInstance();
		void CreateSurface();
		void PickPhysicalDevice();
		std::vector<const char*> GetRequiredExtensions();
		void CreateLogicalDevice();
		void CreateSwapChain();
		void CreateImageViews();
		void CreateShaderModule();
		void CreateGraphicsPipeline();
		void CreateCommandPool();
		void CreateCommandBuffer();
		void RecordCommandBuffer(uint32_t imageIndex);
		void CreateSyncObjects();

		vk::SurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
		vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
		vk::Extent2D ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

		void transition_image_layout(uint32_t currentFrame, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
			vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask);

	private:
		vk::raii::Context m_Context;
		vk::raii::Instance m_Instance = nullptr;
		vk::raii::SurfaceKHR m_Surface = nullptr;
		vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
		vk::raii::Device m_Device = nullptr;
		vk::raii::Device m_LogicalDevice = nullptr;
		vk::raii::PipelineLayout m_PipelineLayout = nullptr;
		vk::raii::Pipeline m_GraphicsPipeline = nullptr;
		vk::raii::CommandPool m_CommandPool = nullptr;
		vk::raii::CommandBuffer m_CommandBuffer = nullptr;
		vk::raii::Semaphore m_PresentCompleteSemaphore = nullptr;
		vk::raii::Semaphore m_RenderFinishedSemaphore = nullptr;
		vk::raii::Fence m_DrawFence = nullptr;
		vk::raii::Queue m_Queue = nullptr;
		uint32_t m_QueueIndex = 0;
		uint32_t m_ImageCount = 0;

		vk::raii::SwapchainKHR m_SwapChain = nullptr;
		std::vector<vk::Image> m_SwapChainImages;
		vk::SurfaceFormatKHR m_SwapChainSurfaceFormat;
		vk::Extent2D m_SwapChainExtent;
		std::vector<vk::raii::ImageView> m_SwapChainImageViews;

		std::vector<const char*> m_RequiredDeviceExtensions = {
			vk::KHRSwapchainExtensionName,
			vk::KHRSpirv14ExtensionName,
			vk::KHRSynchronization2ExtensionName,
			vk::KHRCreateRenderpass2ExtensionName
		};

		std::vector<char> m_ShaderCode;
		vk::raii::ShaderModule m_ShaderModule = nullptr;
		std::vector<vk::DynamicState> m_DynamicStates = {
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

	};
}