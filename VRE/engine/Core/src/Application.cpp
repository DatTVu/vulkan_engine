#include <Application.h>
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace VRE
{
	constexpr uint32_t s_WINDOW_WIDTH = 1920;
	constexpr uint32_t s_WINDOW_HEIGHT = 1080;
	const std::string s_WINDOW_TITLE = "VULKAN RENDERING ENGINE";

	template<> Application* Singleton<Application>::s_Instance = nullptr;

	void Application::Init() 
	{
		m_Window = std::make_unique<Window>(WindowInfo(s_WINDOW_TITLE, s_WINDOW_WIDTH, s_WINDOW_HEIGHT));
		m_Renderer = std::make_unique<Renderer>();
		m_Renderer->Init();
	}

	void Application::Run()
	{
		Init();
		MainLoop();
		Shutdown();
	}
	
	void Application::MainLoop()
	{
		while (!glfwWindowShouldClose(m_Window->GetNativeWindow())) {
			glfwPollEvents();
			m_Renderer->Run();
		}
	}

	void Application::Shutdown()
	{

	}
}

