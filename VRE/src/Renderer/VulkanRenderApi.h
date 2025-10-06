#pragma once

#include "RenderApi.h"

namespace VRE
{
	class VulkanRenderApi : public RenderApi 
	{
		public:
			virtual void Init() override;
			virtual void CleanUp() override;
	};
}