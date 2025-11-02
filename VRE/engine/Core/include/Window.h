#pragma once

#include <cstdint>
#include <string>
#include <Singleton.h>

struct GLFWwindow;

namespace VRE {
	struct WindowInfo {
		uint32_t Width = 1920;
		uint32_t Height = 1080;
		std::string Title = "Untitled";

		WindowInfo() = default;
		WindowInfo(const std::string& tittle, uint32_t width, uint32_t height)
			: Title(tittle), Width(width), Height(height)
		{}
	};

	class Window : public Singleton<Window>{
		public:
			Window(const WindowInfo& Info);
			~Window();

			uint32_t GetWidth() const { return m_Info.Width; }
			uint32_t GetHeight() const { return m_Info.Width; }
			GLFWwindow* GetNativeWindow() { return m_Window; }
			void Shutdown();
			void Init(const WindowInfo& Info);

		private:
			WindowInfo m_Info;
			GLFWwindow* m_Window = nullptr;
	};
}