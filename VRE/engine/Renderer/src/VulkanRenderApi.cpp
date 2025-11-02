//#define VULKAN_HPP_NO_CONSTRUCTORS

#include <VulkanRenderApi.h>
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
#include <ranges>
#include <cstdlib>
#include <memory>
#include <algorithm>


#include <vulkan/vulkan.hpp>

#ifdef __INTELLISENSE__
#include <vulkan/vulkan_raii.hpp>
#endif

#include <vulkan/vk_platform.h>

#define GLFW_INCLUDE_VULKAN // REQUIRED only for GLFW CreateWindowSurface.
#include <GLFW/glfw3.h>

#include <Window.h>

namespace VRE
{
    const std::vector validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };

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
        if (std::ranges::any_of(requiredLayers, [&layerProperties](auto const& requiredLayer) {
            return std::ranges::none_of(layerProperties,
                                   [requiredLayer](auto const& layerProperty)
                                   { return strcmp(layerProperty.layerName, requiredLayer) == 0; });
        }))
        {
            throw std::runtime_error("One or more required layers are not supported!");
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

            auto features = device.template getFeatures2<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>();
            bool supportsRequiredFeatures = features.template get<vk::PhysicalDeviceVulkan13Features>().dynamicRendering &&
                                            features.template get<vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT>().extendedDynamicState;

            return supportsVulkan1_3 && supportsGraphics && supportsAllRequiredExtensions && supportsRequiredFeatures;
        });

        if ( devIter != devices.end() )
        {
            m_PhysicalDevice = *devIter;
        }
        else
        {
            throw std::runtime_error( "failed to find a suitable GPU!" );
        }
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

	void VulkanRenderApi::CreateLogicalDevice()
	{
        // find the index of the first queue family that supports graphics
        std::vector<vk::QueueFamilyProperties> queueFamilyProperties = m_PhysicalDevice.getQueueFamilyProperties();

        // get the first index into queueFamilyProperties which supports graphics
        auto graphicsQueueFamilyProperty = std::ranges::find_if(queueFamilyProperties, [](auto const & qfp)
                        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); } );
		assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");
        
        m_GraphicsQueueFamilyIndex = static_cast<uint32_t>(std::distance(queueFamilyProperties.begin(), graphicsQueueFamilyProperty));

        // determine a queueFamilyIndex that supports present
        // first check if the graphicsIndex is good enough
        m_PresentQueueFamilyIndex = m_PhysicalDevice.getSurfaceSupportKHR(m_GraphicsQueueFamilyIndex, *m_Surface)
                                       ? m_GraphicsQueueFamilyIndex
                                       : static_cast<uint32_t>(queueFamilyProperties.size());

        if (m_PresentQueueFamilyIndex == queueFamilyProperties.size())
        {
            // the graphicsIndex doesn't support present -> look for another family index that supports both
            // graphics and present
            for (size_t i = 0; i < queueFamilyProperties.size(); i++)
            {
                if ((queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eGraphics) && 
                    m_PhysicalDevice.getSurfaceSupportKHR( static_cast<uint32_t>(i), *m_Surface))
                {
                    m_GraphicsQueueFamilyIndex = static_cast<uint32_t>(i);
                    m_PresentQueueFamilyIndex  = m_GraphicsQueueFamilyIndex;
                    break;
                }
            }

            if (m_PresentQueueFamilyIndex == queueFamilyProperties.size())
            {
                // there's nothing like a single family index that supports both graphics and present -> look for another
                // family index that supports present
                for (size_t i = 0; i < queueFamilyProperties.size(); ++i)
                {
                    if (m_PhysicalDevice.getSurfaceSupportKHR(static_cast<uint32_t>(i), *m_Surface))
                    {
                        m_PresentQueueFamilyIndex = static_cast<uint32_t>(i);
                        break;
                    }
                }
            }
        }

        if ((m_GraphicsQueueFamilyIndex == queueFamilyProperties.size()) || (m_PresentQueueFamilyIndex == queueFamilyProperties.size()))
        {
            throw std::runtime_error( "Could not find a queue for graphics or present -> terminating" );
        }

        // query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},                               // vk::PhysicalDeviceFeatures2
            {.dynamicRendering = VK_TRUE },      // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = VK_TRUE }   // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

        // create a Device
        float queuePriority = 0.0f;
        
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = m_GraphicsQueueFamilyIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(m_RequiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = m_RequiredDeviceExtensions.data()
        };
        
        m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
        //TODO: what if graphics and present queue are at same index? do we need to create both queues?
        m_GraphicsQueue = vk::raii::Queue( m_Device, m_GraphicsQueueFamilyIndex, 0 );
        m_PresentQueue = vk::raii::Queue( m_Device, m_PresentQueueFamilyIndex, 0 );
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

    void VulkanRenderApi::CreateSwapChain()
    {
		auto surfaceCapabilities = m_PhysicalDevice.getSurfaceCapabilitiesKHR(m_Surface);
		//TO:DO: move surfaceFormat to member variable?
		ChooseSwapSurfaceFormat(m_PhysicalDevice.getSurfaceFormatsKHR(m_Surface));
		ChooseSwapExtent(surfaceCapabilities);
		auto minImageCount = std::max( 3u, surfaceCapabilities.minImageCount );
		minImageCount = ( surfaceCapabilities.maxImageCount > 0 && minImageCount > surfaceCapabilities.maxImageCount ) ?
			surfaceCapabilities.maxImageCount : minImageCount;
		m_ImageCount = surfaceCapabilities.minImageCount + 1;
		if (surfaceCapabilities.maxImageCount > 0 && m_ImageCount > surfaceCapabilities.maxImageCount)
		{
			m_ImageCount = surfaceCapabilities.maxImageCount;
		}

		vk::SwapchainCreateInfoKHR swapChainCreateInfo {
			.flags = vk::SwapchainCreateFlagsKHR(),
			.surface = m_Surface,
			.minImageCount = minImageCount,
			.imageFormat = m_SwapChainSurfaceFormat.format,
			.imageColorSpace = m_SwapChainSurfaceFormat.colorSpace,
			.imageExtent = m_SwapChainExtent,
			.imageArrayLayers = 1,
			.imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
			.imageSharingMode = vk::SharingMode::eExclusive,
			.preTransform = surfaceCapabilities.currentTransform,
			.compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
			.presentMode = ChooseSwapPresentMode(m_PhysicalDevice.getSurfacePresentModesKHR(m_Surface)),
			.clipped = true,
			.oldSwapchain = VK_NULL_HANDLE
		};

		uint32_t queueFamilyIndices[] = {m_GraphicsQueueFamilyIndex, m_PresentQueueFamilyIndex};
		if (m_GraphicsQueueFamilyIndex != m_PresentQueueFamilyIndex)
		{
			swapChainCreateInfo.imageSharingMode = vk::SharingMode::eConcurrent;
			swapChainCreateInfo.queueFamilyIndexCount = 2;
			swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			swapChainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
			swapChainCreateInfo.queueFamilyIndexCount = 0; // Optional
			swapChainCreateInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		m_SwapChain = vk::raii::SwapchainKHR(m_Device, swapChainCreateInfo);
		m_SwapChainImages = m_SwapChain.getImages();

	}

    void VulkanRenderApi::ChooseSwapSurfaceFormat(const std::vector<vk::SurfaceFormatKHR>& availableFormats)
    {
	    for (const auto& availableFormat : availableFormats)
	    {
            if (availableFormat.format == vk::Format::eB8G8R8A8Srgb && availableFormat.colorSpace == vk::ColorSpaceKHR::eSrgbNonlinear)
            {
                m_SwapChainSurfaceFormat = availableFormat;
            	return;
            }
	    }

        m_SwapChainSurfaceFormat = availableFormats[0];
	}

    vk::PresentModeKHR VulkanRenderApi::ChooseSwapPresentMode(const std::vector<vk::PresentModeKHR>& availablePresentModes)
    {
	    for (const auto& availablePresentMode : availablePresentModes)
	    {
			if (availablePresentMode == vk::PresentModeKHR::eMailbox)
			{
				return availablePresentMode;
			}
	    }
	    return vk::PresentModeKHR::eFifo;
	}

	void VulkanRenderApi::ChooseSwapExtent(const vk::SurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			m_SwapChainExtent = capabilities.currentExtent;
			return;
		}

		if (!VRE::Window::GetPtr())
		{
			throw std::runtime_error("Window not created before creating Vulkan surface!");
		}

		int width, height;
		glfwGetFramebufferSize(VRE::Window::GetPtr()->GetNativeWindow(), &width, &height);
		m_SwapChainExtent =  {
			std::clamp<uint32_t>(width, capabilities.minImageExtent.height, capabilities.maxImageExtent.width),
			std::clamp<uint32_t>(height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height)
		};
	}

	void VulkanRenderApi::CleanUp()
	{
	}
}