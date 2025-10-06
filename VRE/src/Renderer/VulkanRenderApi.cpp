#include "VulkanRenderApi.h"

namespace VRE
{
	void VulkanRenderApi::Init()
	{
		m_API = VRE::RenderApi::API::Vulkan;
	}

	void VulkanRenderApi::CleanUp()
	{

	}
}