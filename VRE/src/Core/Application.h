#pragma once

#include <memory>
#include "../Core/Window.h"
#include "Singleton.h"

namespace VRE
{
	class Application : public Singleton<Application>{
		public:
			void Run();			

		private:
			void Init();
			void MainLoop();
			void Shutdown();

		private:
			std::unique_ptr<Window> m_Window;
	};
}
