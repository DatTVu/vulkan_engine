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
		
		private:
			vk::raii::Context m_Context;
			vk::raii::Instance m_Instance = nullptr;
			vk::raii::PhysicalDevice m_PhysicalDevice = nullptr;
			vk::raii::Device m_LogicalDevice = nullptr;

			vk::raii::Device m_Device = nullptr;
			vk::raii::Queue m_GraphicsQueue = nullptr;
			vk::raii::Queue m_PresentQueue = nullptr;

			vk::raii::SurfaceKHR m_Surface = nullptr;

			std::vector<const char*> m_RequiredDeviceExtensions = {
				vk::KHRSwapchainExtensionName,
				vk::KHRSpirv14ExtensionName,
				vk::KHRSynchronization2ExtensionName,
				vk::KHRCreateRenderpass2ExtensionName
			};
	};
}