//#define VULKAN_HPP_NO_CONSTRUCTORS

#include <../include/VulkanRenderApi.h>
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <ranges>
#include <algorithm>

#include <vulkan/vulkan.hpp>
#include <FileReader.h>

#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#endif

#include <vulkan/vk_platform.h>

#define GLFW_INCLUDE_VULKAN // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>

#include <Window.h>

namespace VRE
{
	static uint32_t ChooseSwapMinImageCount(vk::SurfaceCapabilitiesKHR const & surfaceCapabilities) {
		auto minImageCount = std::max( 3u, surfaceCapabilities.minImageCount );
		if ((0 < surfaceCapabilities.maxImageCount) && (surfaceCapabilities.maxImageCount < minImageCount)) {
			minImageCount = surfaceCapabilities.maxImageCount;
		}
		return minImageCount;
	}

    const std::vector validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

	const std::string k_VulkanShaderPath = {"../VRE/slang.spv"};

    #ifdef NDEBUG
    constexpr bool s_bEnableValidationLayers = false;
    #else
    constexpr bool s_bEnableValidationLayers = true;
    #endif

	void VulkanRenderApi::Init()
	{
		m_API = VRE::RenderApi::API::Vulkan;
		CreateInstance();
        CreateSurface();
		PickPhysicalDevice(); 
        CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateGraphicsPipeline();
		CreateCommandPool();
		CreateCommandBuffer();
		CreateSyncObjects();
	}

	void VulkanRenderApi::CreateInstance()
	{
        vk::ApplicationInfo appInfo{
            .pApplicationName = "Vulkan Rendering Application",
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName = "No Engine",
            .engineVersion = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion = vk::ApiVersion14,
        };
       

        // Get the required layers
        std::vector<char const*> requiredLayers;
        if (s_bEnableValidationLayers) {
            requiredLayers.assign(validationLayers.begin(), validationLayers.end());
        }

        // Check if the required layers are supported by the Vulkan implementation.
        auto layerProperties = m_Context.enumerateInstanceLayerProperties();
		for (auto const& requiredLayer : requiredLayers)
		{
			if (std::ranges::none_of(layerProperties,
									 [requiredLayer](auto const& layerProperty)
									 { return strcmp(layerProperty.layerName, requiredLayer) == 0; }))
			{
				throw std::runtime_error("Required layer not supported: " + std::string(requiredLayer));
			}
		}

        // Get the required instance extensions from GLFW.
        auto requiredExtensions = GetRequiredExtensions();

        // Check if the required GLFW extensions are supported by the Vulkan implementation.
        auto extensionProperties = m_Context.enumerateInstanceExtensionProperties();
        for (auto const& requiredExtension : requiredExtensions)
        {
            if (std::ranges::none_of(extensionProperties,
                                     [requiredExtension](auto const& extensionProperty)
                                     { return strcmp(extensionProperty.extensionName, requiredExtension) == 0; }))
            {
                throw std::runtime_error("Required extension not supported: " + std::string(requiredExtension));
            }
        }

        vk::InstanceCreateInfo createInfo{
			.pApplicationInfo = &appInfo,
			.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size()),
			.ppEnabledLayerNames = requiredLayers.data(),
			.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
			.ppEnabledExtensionNames = requiredExtensions.data()
        };
      
        m_Instance = vk::raii::Instance(m_Context, createInfo);
	}

	void VulkanRenderApi::CreateSurface()
	{
		//TODO: Generalize this for other windowing systems
		if (!VRE::Window::GetPtr()) {
			throw std::runtime_error("Window not created before creating Vulkan surface!");
		}
		GLFWwindow* window = VRE::Window::GetPtr()->GetNativeWindow();
		VkSurfaceKHR _surface;
		if (glfwCreateWindowSurface(*m_Instance, window, nullptr, &_surface) != 0)
		{
			throw std::runtime_error("failed to create window surface!");
		}
		m_Surface = vk::raii::SurfaceKHR(m_Instance, _surface);
	}

    void VulkanRenderApi::PickPhysicalDevice() {
        std::vector<vk::raii::PhysicalDevice> devices = m_Instance.enumeratePhysicalDevices();
        const auto devIter = std::ranges::find_if(devices,
        [&](auto const & device) {
               // Check if the device supports the Vulkan 1.3 API version
            bool supportsVulkan1_3 = device.getProperties().apiVersion >= VK_API_VERSION_1_3;

            // Check if any of the queue families support graphics operations
            auto queueFamilies = device.getQueueFamilyProperties();
            bool supportsGraphics =
              std::ranges::any_of( queueFamilies, []( auto const & qfp ) { return !!( qfp.queueFlags & vk::QueueFlagBits::eGraphics ); } );

            // Check if all required device extensions are available
            auto availableDeviceExtensions = device.enumerateDeviceExtensionProperties();
            bool supportsAllRequiredExtensions =
              std::ranges::all_of(m_RequiredDeviceExtensions,
                [&availableDeviceExtensions]( auto const & requiredDeviceExtension )
                {
                    return std::ranges::any_of( availableDeviceExtensions,
                                                [requiredDeviceExtension]( auto const & availableDeviceExtension )
                                                { return strcmp( availableDeviceExtension.extensionName, requiredDeviceExtension ) == 0; } );
                });

        	auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2,
														 vk::PhysicalDeviceVulkan11Features,
														 vk::PhysicalDeviceVulkan13Features,
														 vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
        	bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan11Features>().shaderDrawParameters &&
										   features.template get<vk::PhysicalDeviceVulkan13Features>().synchronization2 &&
										   features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
										   features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        });

        if (devIter != devices.end())
        {
            m_PhysicalDevice = *devIter;
        }
        else
        {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

	void VulkanRenderApi::CreateLogicalDevice()
	{
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports both graphics and present
        for (uint32_t qfpIndex = 0; qfpIndex < queueFamilyProperties.size(); qfpIndex++)
        {
            if ((queueFamilyProperties[qfpIndex].queueFlags & vk::QueueFlagBits::eGraphics) &&
                m_PhysicalDevice.getSurfaceSupportKHR(qfpIndex, *m_Surface))
            {
                // found a queue family that supports both graphics and present
                m_QueueIndex = qfpIndex;
                break;
            }
        }
        if (m_QueueIndex == ~0)
        {
            throw std::runtime_error("Could not find a queue for graphics and present -> terminating");
        }

        // query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2,
                           vk::PhysicalDeviceVulkan11Features,
                           vk::PhysicalDeviceVulkan13Features,
                           vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>
          featureChain = {
            {},                                                     // vk::PhysicalDeviceFeatures2
            {.shaderDrawParameters = true },                        // vk::PhysicalDeviceVulkan11Features
            {.synchronization2 = true, .dynamicRendering = true },  // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = true }                         // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

        // create a Device
        float                     queuePriority = 0.0f;
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo{ .queueFamilyIndex = m_QueueIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };
        vk::DeviceCreateInfo      deviceCreateInfo{ .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
                                                    .queueCreateInfoCount = 1,
                                                    .pQueueCreateInfos = &deviceQueueCreateInfo,
                                                    .enabledExtensionCount = static_cast<uint32_t>(m_RequiredDeviceExtensions.size()),
                                                    .ppEnabledExtensionNames = m_RequiredDeviceExtensions.data() };

        m_Device = vk::raii::Device( m_PhysicalDevice, deviceCreateInfo );
        m_Queue = vk::raii::Queue( m_Device, m_QueueIndex, 0 );
	}

	void VulkanRenderApi::CreateSwapChain()
	{
		auto surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(*m_Surface);
		m_SwapChainExtent          = ChooseSwapExtent(surfaceCapabilities);
		m_SwapChainSurfaceFormat   = ChooseSwapSurfaceFormat(m_PhysicalDevice.getSurfaceFormatsKHR(*m_Surface));
		vk::SwapchainCreateInfoKHR swapChainCreateInfo{ .surface          = *m_Surface,
														.minImageCount    = ChooseSwapMinImageCount(surfaceCapabilities),
														.imageFormat      = m_SwapChainSurfaceFormat.format,
														.imageColorSpace  = m_SwapChainSurfaceFormat.colorSpace,
														.imageExtent      = m_SwapChainExtent,
														.imageArrayLayers = 1,
														.imageUsage       = vk::ImageUsageFlagBits::eColorAttachment,
														.imageSharingMode = vk::SharingMode::eExclusive,
														.preTransform     = surfaceCapabilities.currentTransform,
														.compositeAlpha   = vk::CompositeAlphaFlagBitsKHR::eOpaque,
														.presentMode      = ChooseSwapPresentMode(m_PhysicalDevice.getSurfacePresentModesKHR(*m_Surface)),
														.clipped          = true };

		m_SwapChain = vk::raii::SwapchainKHR( m_Device, swapChainCreateInfo );
		m_SwapChainImages = m_SwapChain.getImages();
	}

	void VulkanRenderApi::CreateGraphicsPipeline()
	{
		CreateShaderModule();
		if (m_ShaderModule != nullptr)
		{
			vk::PipelineShaderStageCreateInfo vertShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eVertex, .module = m_ShaderModule, .pName = "vertMain" };

			vk::PipelineShaderStageCreateInfo fragShaderStageInfo{ .stage = vk::ShaderStageFlagBits::eFragment, .module = m_ShaderModule, .pName = "fragMain" };

			vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

			//TO-DO: move input assembly to a member variable and config primitive topology
			vk::PipelineVertexInputStateCreateInfo vertexInputInfo;

			vk::PipelineInputAssemblyStateCreateInfo inputAssembly = { .topology = vk::PrimitiveTopology::eTriangleList };
			vk::PipelineViewportStateCreateInfo viewportState{ .viewportCount = 1, .scissorCount = 1 };
			vk::Rect2D rect = { vk::Offset2D{0,0}, m_SwapChainExtent };

			vk::PipelineRasterizationStateCreateInfo rasterizer{
				.depthClampEnable = vk::False, .rasterizerDiscardEnable = vk::False,
				.polygonMode = vk::PolygonMode::eFill, .cullMode = vk::CullModeFlagBits::eBack,
				.frontFace = vk::FrontFace::eClockwise, .depthBiasEnable = vk::False,
				.depthBiasSlopeFactor = 1.0f, .lineWidth = 1.0f
			};

			vk::PipelineMultisampleStateCreateInfo multisampling{.rasterizationSamples = vk::SampleCountFlagBits::e1, .sampleShadingEnable = vk::False};

			vk::PipelineColorBlendAttachmentState colorBlendAttachment{ .blendEnable = vk::False,
			.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA
			};

			vk::PipelineColorBlendStateCreateInfo colorBlending{.logicOpEnable = vk::False, .logicOp =  vk::LogicOp::eCopy, .attachmentCount = 1, .pAttachments =  &colorBlendAttachment };

			//TO-DO: read about dynamic state. what can be config?
			vk::PipelineDynamicStateCreateInfo dynamicState = {
				.dynamicStateCount = static_cast<uint32_t>(m_DynamicStates.size()),
				.pDynamicStates = m_DynamicStates.data()
			};

			vk::PipelineLayoutCreateInfo pipelineLayoutInfo{  .setLayoutCount = 0, .pushConstantRangeCount = 0 };

			m_PipelineLayout = vk::raii::PipelineLayout(m_Device, pipelineLayoutInfo);

			vk::StructureChain<vk::GraphicsPipelineCreateInfo, vk::PipelineRenderingCreateInfo> pipelineCreateInfoChain = {
				{.stageCount = 2,
				  .pStages = shaderStages,
				  .pVertexInputState = &vertexInputInfo,
				  .pInputAssemblyState = &inputAssembly,
				  .pViewportState = &viewportState,
				  .pRasterizationState = &rasterizer,
				  .pMultisampleState = &multisampling,
				  .pColorBlendState = &colorBlending,
				  .pDynamicState = &dynamicState,
				  .layout = m_PipelineLayout,
				  .renderPass = nullptr },
				{.colorAttachmentCount = 1, .pColorAttachmentFormats = &m_SwapChainSurfaceFormat.format }
			};

			m_GraphicsPipeline =  vk::raii::Pipeline(m_Device, nullptr, pipelineCreateInfoChain.get<vk::GraphicsPipelineCreateInfo>());
		}
	}

	void VulkanRenderApi::CreateCommandPool()
	{
		vk::CommandPoolCreateInfo poolInfo{ .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer, .queueFamilyIndex = m_QueueIndex };
		m_CommandPool = vk::raii::CommandPool(m_Device, poolInfo);
	}

	void VulkanRenderApi::CreateCommandBuffer()
	{
		vk::CommandBufferAllocateInfo allocInfo{ .commandPool = m_CommandPool, .level = vk::CommandBufferLevel::ePrimary, .commandBufferCount = 1 };

		m_CommandBuffer = std::move(vk::raii::CommandBuffers(m_Device, allocInfo).front());
	}

	void VulkanRenderApi::RecordCommandBuffer(uint32_t imageIndex)
	{
		m_CommandBuffer.begin( {} );
        // Before starting rendering, transition the swapchain image to COLOR_ATTACHMENT_OPTIMAL
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eUndefined,
            vk::ImageLayout::eColorAttachmentOptimal,
            {},                                                         // srcAccessMask (no need to wait for previous operations)
            vk::AccessFlagBits2::eColorAttachmentWrite,                 // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
            vk::PipelineStageFlagBits2::eColorAttachmentOutput          // dstStage
        );

		const vk::ClearColorValue clearColorValue {std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}};
        vk::RenderingAttachmentInfo attachmentInfo = {
            .imageView = m_SwapChainImageViews[imageIndex],
            .imageLayout = vk::ImageLayout::eColorAttachmentOptimal,
            .loadOp = vk::AttachmentLoadOp::eClear,
            .storeOp = vk::AttachmentStoreOp::eStore,
            .clearValue = static_cast<vk::ClearValue>(clearColorValue)
        };

        const vk::RenderingInfo renderingInfo = {
            .renderArea = { .offset = { 0, 0 }, .extent = m_SwapChainExtent },
            .layerCount = 1,
            .colorAttachmentCount = 1,
            .pColorAttachments = &attachmentInfo
        };

        m_CommandBuffer.beginRendering(renderingInfo);
        m_CommandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *m_GraphicsPipeline);
        m_CommandBuffer.setViewport(0, vk::Viewport(0.0f, 0.0f, static_cast<float>(m_SwapChainExtent.width), static_cast<float>(m_SwapChainExtent.height), 0.0f, 1.0f));
        m_CommandBuffer.setScissor( 0, vk::Rect2D( vk::Offset2D( 0, 0 ), m_SwapChainExtent ) );
        m_CommandBuffer.draw(3, 1, 0, 0);
        m_CommandBuffer.endRendering();
        // After rendering, transition the swapchain image to PRESENT_SRC
        transition_image_layout(
            imageIndex,
            vk::ImageLayout::eColorAttachmentOptimal,
            vk::ImageLayout::ePresentSrcKHR,
            vk::AccessFlagBits2::eColorAttachmentWrite,                 // srcAccessMask
            {},                                                         // dstAccessMask
            vk::PipelineStageFlagBits2::eColorAttachmentOutput,         // srcStage
            vk::PipelineStageFlagBits2::eBottomOfPipe                   // dstStage
        );
        m_CommandBuffer.end();
	}

	void VulkanRenderApi::transition_image_layout(uint32_t currentFrame, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::AccessFlags2 src_access_mask,
			vk::AccessFlags2 dst_access_mask, vk::PipelineStageFlags2 src_stage_mask, vk::PipelineStageFlags2 dst_stage_mask)
	{
		vk::ImageMemoryBarrier2 barrier = {
			.srcStageMask = src_stage_mask,
			.srcAccessMask = src_access_mask,
			.dstStageMask = dst_stage_mask,
			.dstAccessMask = dst_access_mask,
			.oldLayout = old_layout,
			.newLayout = new_layout,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = m_SwapChainImages[currentFrame],
			.subresourceRange = {
				.aspectMask = vk::ImageAspectFlagBits::eColor,
				.baseMipLevel = 0,
				.levelCount = 1,
				.baseArrayLayer = 0,
				.layerCount = 1
			}
		};
		vk::DependencyInfo dependency_info = {
			.dependencyFlags = {},
			.imageMemoryBarrierCount = 1,
			.pImageMemoryBarriers = &barrier
		};
		m_CommandBuffer.pipelineBarrier2(dependency_info);
	}

	void VulkanRenderApi::CreateSyncObjects()
	{
		m_PresentCompleteSemaphore = vk::raii::Semaphore(m_Device, vk::SemaphoreCreateInfo());
		m_RenderFinishedSemaphore = vk::raii::Semaphore(m_Device, vk::SemaphoreCreateInfo());
		m_DrawFence = vk::raii::Fence(m_Device, {.flags = vk::FenceCreateFlagBits::eSignaled});
	}

	void VulkanRenderApi::DrawFrame()
	{
		//Wait for the previous frame to finish
		m_Queue.waitIdle();
		//Acquire an image from the swap chain
		auto [result, imageIndex] = m_SwapChain.acquireNextImage(UINT64_MAX, *m_PresentCompleteSemaphore, nullptr);
		//Record a command buffer which draws the scene onto that image
		RecordCommandBuffer(imageIndex);
		m_Device.resetFences(*m_DrawFence);
		//Submit the recorded command buffer
		vk::PipelineStageFlags waitDestinationStageMask( vk::PipelineStageFlagBits::eColorAttachmentOutput );

		const vk::SubmitInfo submitInfo{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*m_PresentCompleteSemaphore,
							.pWaitDstStageMask = &waitDestinationStageMask, .commandBufferCount = 1, .pCommandBuffers = &*m_CommandBuffer,
							.signalSemaphoreCount = 1, .pSignalSemaphores = &*m_RenderFinishedSemaphore };

		m_Queue.submit(submitInfo, *m_DrawFence);

		while ( vk::Result::eTimeout == m_Device.waitForFences( *m_DrawFence, vk::True, UINT64_MAX ) )
			;

		//Present the swap chain image
		const vk::PresentInfoKHR presentInfoKHR{ .waitSemaphoreCount = 1, .pWaitSemaphores = &*m_RenderFinishedSemaphore,
												.swapchainCount = 1, .pSwapchains = &*m_SwapChain, .pImageIndices = &imageIndex };
		result = m_Queue.presentKHR( presentInfoKHR );
		switch ( result )
		{
			case vk::Result::eSuccess: break;
			case vk::Result::eSuboptimalKHR: std::cout << "vk::Queue::presentKHR returned vk::Result::eSuboptimalKHR !\n"; break;
			default: break;  // an unexpected result is returned!
		}
	}

	void VulkanRenderApi::CreateImageViews() {
		assert(m_SwapChainImageViews.empty());

		vk::ImageViewCreateInfo imageViewCreateInfo{ .viewType = vk::ImageViewType::e2D, .format = m_SwapChainSurfaceFormat.format,
		  .subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 } };
		for (auto& image : m_SwapChainImages)
		{
			imageViewCreateInfo.image = image;
			m_SwapChainImageViews.emplace_back(m_Device, imageViewCreateInfo);
		}
	}

	void VulkanRenderApi::CreateShaderModule()
	{
		//TO-DO: inject cmake shader build dir into here
		m_ShaderCode = FileReader::ReadShaderFile(k_VulkanShaderPath);

		vk::ShaderModuleCreateInfo createInfo {
			.codeSize = m_ShaderCode.size() * sizeof(char),
			.pCode = reinterpret_cast<const uint32_t*>(m_ShaderCode.data())
		};

		m_ShaderModule = vk::raii::ShaderModule(m_Device, createInfo);
	}

	vk::SurfaceFormatKHR VulkanRenderApi::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
	{
		assert(!availableFormats.empty());
		const auto formatIt = std::ranges::find_if(
			availableFormats,
			[]( const auto & format ) { return format.format == vk::Format::eB8G8R8A8Srgb && format.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear; } );
		return formatIt != availableFormats.end() ? *formatIt : availableFormats[0];
	}

	vk::PresentModeKHR VulkanRenderApi::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
	{
		assert(std::ranges::any_of(availablePresentModes, [](auto presentMode){ return presentMode == vk::PresentModeKHR::eFifo; }));
		return std::ranges::any_of(availablePresentModes,
			[](const vk::PresentModeKHR value) { return vk::PresentModeKHR::eMailbox == value; } ) ? vk::PresentModeKHR::eMailbox : vk::PresentModeKHR::eFifo;
	}

	vk::Extent2D VulkanRenderApi::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			m_SwapChainExtent = capabilities.currentExtent;
			return capabilities.currentExtent;
		}

		if (!VRE::Window::GetPtr())
		{
			throw std::runtime_error("Window not created before creating Vulkan surface!");
		}

		int width, height;
		glfwGetFramebufferSize(VRE::Window::GetPtr()->GetNativeWindow(), &width, &height);
		return {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

    std::vector<const char*> VulkanRenderApi::GetRequiredExtensions()
    {
        uint32_t glfwExtensionCount = 0;
        auto glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
        if (s_bEnableValidationLayers) {
            extensions.push_back(vk::EXTDebugUtilsExtensionName );
        }

        return extensions;
    }

	void VulkanRenderApi::CleanUp()
	{
		m_Device.waitIdle();
	}
}