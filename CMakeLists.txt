cmake_minimum_required(VERSION 3.26 FATAL_ERROR)

project(demo)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_MSAN "Enable MemorySanitizer" OFF)
option(ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(ENABLE_COMPILER_WARNING "Enable COMPILER WARNING" OFF)
option(ENABLE_CPP20_MODULE "Enable C++ 20 module support for Vulkan" OFF)

if(ENABLE_CPP20_MODULE)
    set(CMAKE_CXX_SCAN_FOR_MODULES ON)
endif()

#enable clang-tidy
find_program (CLANG_TIDY_EXE NAMES "clang-tidy")
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE};-format-style=file;-header-filter='${CMAKE_SOURCE_DIR}/*'" CACHE STRING "" FORCE)
else()
    message(STATUS "clang-tidy NOT found!")
    set(CMAKE_CXX_CLANG_TIDY "" CACHE STRING "" FORCE) # clear it
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    if(ENABLE_COMPILER_WARNING)
        add_compile_options(-Wall -Wextra -Werror)
    endif()
    
    if (ENABLE_MSAN)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=undefined")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=memory")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory")
    endif()

    if(ENABLE_TSAN)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=thread")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
    endif()
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    if(ENABLE_COMPILER_WARNING)
        add_compile_options(/W3)
    endif()
endif()

add_executable(demo)
add_subdirectory(VRE)

target_sources(demo
    PRIVATE
        demo.cpp
)

#link VRE libraries
target_link_libraries(demo
    PRIVATE
        VRE
)

target_link_directories(demo PRIVATE ${GLFW3_LIBRARY})
