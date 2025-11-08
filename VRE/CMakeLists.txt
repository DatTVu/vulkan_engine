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
    target_link_libraries(VRE PRIVATE
            "-framework Cocoa"
            "-framework IOKit"
            "-framework CoreFoundation"
    )
elseif(WINDOWS)
    message("Windows")
endif()

#glfw lib
target_link_libraries(VRE PUBLIC glfw3)
target_include_directories(VRE PUBLIC ${GLFW3_INCLUDE_DIR})

#glm
target_include_directories(VRE PRIVATE libs/glm)

# Define the directory containing the source files
# Use file(GLOB) to find all .cpp files
set(CORE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Core/src")
set(RENDERER_CORE_SOURCE_DIR "engine/Renderer/core/src")
set(RENDERER_VULKAN_SOURCE_DIR "engine/Renderer/platforms/vulkan/src")
set(UTILS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Utils/src")
file(GLOB CORE_SOURCES CONFIGURE_DEPENDS "${CORE_SOURCE_DIR}/*.cpp")
file(GLOB RENDERER_CORE_SOURCES CONFIGURE_DEPENDS "${RENDERER_CORE_SOURCE_DIR}/*.cpp")
file(GLOB RENDERER_VULKAN_SOURCES CONFIGURE_DEPENDS "${RENDERER_VULKAN_SOURCE_DIR}/*.cpp")
file(GLOB UTILS_SOURCES CONFIGURE_DEPENDS "${UTILS_SOURCE_DIR}/*.cpp")

set(CORE_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Core/include")
set(RENDERER_CORE_HEADER_DIR "engine/Renderer/core/include")
set(RENDERER_VULKAN_HEADER_DIR "engine/Renderer/platforms/vulkan/include")
set(UTILS_HEADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/engine/Utils/include")
file(GLOB CORE_HEADERS CONFIGURE_DEPENDS "${CORE_HEADER_DIR}/*.h")
file(GLOB RENDERER_CORE_HEADERS CONFIGURE_DEPENDS "${RENDERER_CORE_HEADER_DIR}/*.h")
file(GLOB RENDERER_VULKAN_HEADERS CONFIGURE_DEPENDS "${RENDERER_VULKAN_HEADER_DIR}/*.h")
file(GLOB UTILS_HEADERS CONFIGURE_DEPENDS "${UTILS_HEADER_DIR}/*.h")

# Add the found source files to the target
target_sources(VRE 
    PRIVATE 
        ${CORE_SOURCES}
        ${RENDERER_CORE_SOURCES}
        ${RENDERER_VULKAN_SOURCES}
        ${UTILS_SOURCES}
    
    PUBLIC 
        FILE_SET HEADERS
        BASE_DIRS 
            ${CORE_HEADER_DIR}
            ${RENDERER_CORE_HEADER_DIR}
            ${RENDERER_VULKAN_HEADER_DIR}
            ${UTILS_HEADER_DIR}

        FILES 
            ${CORE_HEADERS}
            ${RENDERER_CORE_HEADERS}
            ${RENDERER_VULKAN_HEADERS}
            ${UTILS_HEADERS}
)

#find slangc to compile the shaders
find_program(SLANGC_EXECUTABLE slangc)
if(NOT SLANGC_EXECUTABLE)
    message(FATAL_ERROR "slangc executable not found. Please add it to your PATH or set SLANGC_EXECUTABLE.")
endif()

# Function to compile shaders
function(compile_and_add_shaders TARGET_NAME)
    file(GLOB SHADER_SOURCE_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/engine/Renderer/resources/shaders/*.slang")
    message("Shader sources: [${SHADER_SOURCE_FILES}]")
    # Validate that source files have been passed
    list(LENGTH SHADER_SOURCE_FILES FILE_COUNT)
    if(FILE_COUNT EQUAL 0)
        message(FATAL_ERROR "Cannot create a shaders target without any source files")
    else()
        set(SHADER_COMMANDS)
        set(SHADER_PRODUCTS)
        list(APPEND SHADER_COMMANDS "${SLANGC_EXECUTABLE}")

        foreach(SHADER_SOURCE IN LISTS SHADER_SOURCE_FILES)
            cmake_path(ABSOLUTE_PATH SHADER_SOURCE NORMALIZE)
            cmake_path(GET SHADER_SOURCE FILENAME SHADER_NAME)
            message("Adding [${SHADER_SOURCE}]")
            # Build command
            list(APPEND SHADER_COMMANDS "${SHADER_SOURCE}")
            #TO-DO: compile each shader file to a spv then link them into a single big file?
            #list(APPEND SHADER_COMMANDS -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o)
            #list(APPEND SHADER_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")
            # Add product
            #list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/${SHADER_NAME}.spv")
        endforeach()

        list(APPEND SHADER_COMMANDS -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name -entry vertMain -entry fragMain -o)
        list(APPEND SHADER_COMMANDS "${CMAKE_CURRENT_BINARY_DIR}/slang.spv")
        # Add product
        list(APPEND SHADER_PRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/slang.spv")

        add_custom_target(${TARGET_NAME} ALL
                ${SHADER_COMMANDS}
                COMMENT "Compiling Shaders [${TARGET_NAME}]"
                SOURCES ${SHADER_SOURCE_FILES}
                BYPRODUCTS ${SHADER_PRODUCTS}
        )
    endif()
endfunction()

if(SLANGC_EXECUTABLE)
    compile_and_add_shaders(shaders)
    add_dependencies(VRE shaders)
endif()