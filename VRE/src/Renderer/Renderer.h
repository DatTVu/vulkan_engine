#pragma once

namespace VRE {
	class Renderer {
		public:
			Renderer();
			~Renderer();
			void Run();

		private:
			void InitVulkan();
			void CleanUp();
	};
}
