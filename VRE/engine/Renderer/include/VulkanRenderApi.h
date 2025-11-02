#pragma once

#include <RenderApi.h>
#include <vulkan/vulkan_raii.hpp>

namespace VRE
{
	class VulkanRenderApi : public RenderApi 
	{
		public:
			virtual void Init() override;
			virtual void CleanUp() override;

		private:
			void CreateInstance();
			void CreateSurface();
			void PickPhysicalDevice();
			std::vector<const char*> GetRequiredExtensions(); 
			void CreateLogicalDevice();
			void CreateSwapChain();
			void CreateImageViews();
			void ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats);
			vk::PresentModeKHR ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes);
			void ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities);

		private:
			vk::raii::Context m_Context;
			vk::raii::Instance m_Instance = nullptr;
			vk::raii::SurfaceKHR m_Surface = nullptr;
			vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
			vk::raii::Device m_Device = nullptr;
			vk::raii::Device m_LogicalDevice = nullptr;

			vk::raii::Queue m_GraphicsQueue = nullptr;
			uint32_t m_GraphicsQueueFamilyIndex = 0;
			vk::raii::Queue m_PresentQueue = nullptr;
			uint32_t m_PresentQueueFamilyIndex = 0;

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
	};
}