#Find vulkan
find_package(Vulkan REQUIRED)

if(ENABLE_CPP20_MODULE)
    add_library(VulkanCppModule)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)

    target_compile_definitions(VulkanCppModule PUBLIC
            VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1
            VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1
    )

    target_include_directories(VulkanCppModule PRIVATE "${Vulkan_INCLUDE_DIR}")
    target_link_libraries(VulkanCppModule PUBLIC Vulkan::Vulkan)
    set_target_properties(VulkanCppModule PROPERTIES CXX_STANDARD 20)

    # Add MSVC-specific compiler options for proper C++ module support
    if(MSVC)
        target_compile_options(VulkanCppModule PRIVATE
        /std:c++20      # Use latest C++ standard for better module support
        /permissive-        # Standards conformance mode
        /Zc:__cplusplus     # Enable correct __cplusplus macro
        /EHsc               # Enable C++ exception handling
        /Zc:preprocessor    # Use conforming preprocessor
        /translateInclude   # Automatically translate #include to import for standard library
        )
    endif()

    target_sources(VulkanCppModule
            PUBLIC
            FILE_SET cxx_modules TYPE CXX_MODULES
            BASE_DIRS "${Vulkan_INCLUDE_DIR}"
            FILES "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )

    # Add the vulkan.cppm file directly as a source file
    target_sources(VulkanCppModule
          PRIVATE
          "${Vulkan_INCLUDE_DIR}/vulkan/vulkan.cppm"
    )
else()
    # Create a dummy interface library when C++ 20 module is disabled
    add_library(VulkanCppModule INTERFACE)
    add_library(Vulkan::cppm ALIAS VulkanCppModule)
    target_link_libraries(VulkanCppModule INTERFACE Vulkan::Vulkan)
    target_compile_definitions(VulkanCppModule
        INTERFACE VULKAN_HPP_DISPATCH_LOADER_DYNAMIC=1 VULKAN_HPP_NO_STRUCT_CONSTRUCTORS=1 VULKAN_HPP_NO_CONSTRUCTORS=1
    )
endif()    

#Link vulkan to VRE
add_library(VRE)
target_link_libraries(VRE PRIVATE Vulkan::cppm)
target_compile_features(VRE PUBLIC cxx_std_20)

if(MACOS)
    message("MacOS")
elseif(WINDOWS)
    message("Windows")
endif()
    

#glfw lib
target_include_directories(VRE PRIVATE ${GLFW3_INCLUDE_DIR})
target_link_libraries(VRE PRIVATE glfw3)

#glm
target_include_directories(VRE PRIVATE libs/glm)

# Define the directory containing the source files
# Use file(GLOB) to find all .cpp files
set(CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Core/src")
set(RENDERER_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Renderer/src")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "${CORE_SOURCE_DIR}/*.cpp")
file(GLOB RENDERER_SOURCES CONFIGURE_DEPENDS "${RENDERER_SOURCE_DIR}/*.cpp")

# Define the directory containing the header files
# Use file(GLOB) to find all .h files
set(CORE_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Core/include")
set(RENDERER_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Renderer/include")
file(GLOB CORE_HEADERS CONFIGURE_DEPENDS "${CORE_HEADER_DIR}/*.h")
file(GLOB RENDERER_HEADERS CONFIGURE_DEPENDS "${RENDERER_HEADER_DIR}/*.h")

# Add the found source files to the target
target_sources(VRE 
    PRIVATE 
        ${CORE_SOURCES}
        ${RENDERER_SOURCES}
    
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS 
            ${CORE_HEADER_DIR}
            ${RENDERER_HEADER_DIR}
        FILES 
            ${CORE_HEADERS}
            ${RENDERER_HEADERS}
)
