#pragma once

namespace VRE
{
	class RenderApi
	{
		public: 
			enum class API
			{
				None = 0,
				OpenGL = 1,
				Vulkan = 2,
				Max
			};

		public:
			virtual ~RenderApi() = default;
			
			virtual void Init() = 0;
			virtual void CleanUp() = 0;
			API GetAPI() { return m_API; }

		protected:
			API m_API;
	};
}