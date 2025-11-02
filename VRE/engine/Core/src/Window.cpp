#include <Window.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace VRE {

	template<> Window* Singleton<Window>::s_Instance = 0;

	Window::Window(const WindowInfo& Info) 
	{
		Init(Info);
	}

	Window::~Window()
	{
		Shutdown();
	}

	void Window::Init(const WindowInfo& Info)
	{
		m_Info.Height = Info.Height;
		m_Info.Width = Info.Width;
		m_Info.Title = Info.Title;

		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_Window = glfwCreateWindow(m_Info.Width, m_Info.Height, m_Info.Title.c_str() , nullptr, nullptr);
	}

	void Window::Shutdown()
	{
		glfwDestroyWindow(m_Window);
		glfwTerminate();
	}
}