#pragma once

#include <memory>
#include "RenderApi.h"
#include "../Core/Singleton.h"

namespace VRE {
	class Renderer : public Singleton<Renderer>{
		public:
			void Run();
			void Init();

		private:
			void CleanUp();

		private:
			std::unique_ptr<RenderApi> m_RenderApi;
	};
}
