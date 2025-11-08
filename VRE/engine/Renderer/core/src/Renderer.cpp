#include <Renderer.h>
#include <VulkanRenderApi.h>
#include <iostream>

#define USE_VULKAN_RENDER_API 1
#define USE_OPENGL_RENDER_API 0

namespace VRE {

	template<> Renderer* Singleton<Renderer>::s_Instance = nullptr;

	void Renderer::Run() 
	{
		CleanUp();
	}

	void Renderer::Init() 
	{
#ifdef USE_VULKAN_RENDER_API
		m_RenderApi = std::make_unique<VulkanRenderApi>();
#elif USE_OPENGL_RENDER_API
		m_RenderApi = std::make_unique<RenderApi>();
#endif // USE_VULKAN_RENDER_API
		m_RenderApi->Init();
	}

	void Renderer::CleanUp() 
	{
		
	}
}
