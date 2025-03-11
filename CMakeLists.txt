cmake_minimum_required(VERSION 3.5)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_C_COMPILER clang)
set(CMAKE_CXX_COMPILER clang++)

project(VoxelEngine)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}")
set(CMAKE_BUILD_PARALLEL_LEVEL 12)

file(GLOB_RECURSE SOURCES "src/*.cpp")
include_directories(src)
include_directories(lib/STAM)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)


# GLFW
set(BUILD_SHARED_LIBS OFF)           
set(GLFW_VULKAN_STATIC ON)
set(GLFW_LIBRARY_TYPE STATIC)        
set(GLFW_BUILD_EXAMPLES OFF)         
set(GLFW_BUILD_TESTS OFF)            
set(GLFW_BUILD_DOCS OFF)             

add_subdirectory(lib/GLFW)
include_directories(lib/GLFW/include)
set_target_properties(glfw PROPERTIES POSITION_INDEPENDENT_CODE ON)

find_package(Vulkan REQUIRED)

# Platform-specific logic for Vulkan linking
if(WIN32)
    message(STATUS "Configuring for Windows")
    find_package(Vulkan REQUIRED)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK not found!")
    endif()
elseif(APPLE)
    message(STATUS "Configuring for macOS")
    find_package(Vulkan REQUIRED)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "MoltenVK is required for Vulkan on macOS!")
    endif()
    # You may need to add MoltenVK specific includes or libraries
    # Use Vulkan SDK setup from https://vulkan.lunarg.com/
else()
    message(STATUS "Configuring for Linux")
    find_package(Vulkan REQUIRED)
    if (NOT Vulkan_FOUND)
        message(FATAL_ERROR "Vulkan SDK not found!")
    endif()
endif()

add_executable(VoxelEngine ${SOURCES})
target_link_libraries(VoxelEngine glfw Vulkan::Vulkan)
