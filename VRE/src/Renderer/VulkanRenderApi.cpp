#define VULKAN_HPP_NO_CONSTRUCTORS

#include "VulkanRenderApi.h"
#include <assert.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <cstring>
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
		PickPhysicalDevice(); 
        CreateLogicalDevice();
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
        auto graphicsQueueFamilyProperty = std::ranges::find_if( queueFamilyProperties, []( auto const & qfp )
                        { return (qfp.queueFlags & vk::QueueFlagBits::eGraphics) != static_cast<vk::QueueFlags>(0); } );
		assert(graphicsQueueFamilyProperty != queueFamilyProperties.end() && "No graphics queue family found!");

        
        auto graphicsIndex = static_cast<uint32_t>( std::distance( queueFamilyProperties.begin(), graphicsQueueFamilyProperty ) );

        // query for Vulkan 1.3 features
        vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan13Features, vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT> featureChain = {
            {},                               // vk::PhysicalDeviceFeatures2
            {.dynamicRendering = VK_TRUE },      // vk::PhysicalDeviceVulkan13Features
            {.extendedDynamicState = VK_TRUE }   // vk::PhysicalDeviceExtendedDynamicStateFeaturesEXT
        };

        // create a Device
        float queuePriority = 0.0f;
        
        vk::DeviceQueueCreateInfo deviceQueueCreateInfo { .queueFamilyIndex = graphicsIndex, .queueCount = 1, .pQueuePriorities = &queuePriority };

        vk::DeviceCreateInfo deviceCreateInfo{
            .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledExtensionCount = static_cast<uint32_t>(m_RequiredDeviceExtensions.size()),
            .ppEnabledExtensionNames = m_RequiredDeviceExtensions.data()
        };
        
        m_Device = vk::raii::Device(m_PhysicalDevice, deviceCreateInfo);
        m_GraphicsQueue = vk::raii::Queue( m_Device, graphicsIndex, 0 );
	}

	void VulkanRenderApi::CleanUp()
	{

	}
}