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


# ===========SHADERS=================
set(SHADER_SRC_DIR "${CMAKE_SOURCE_DIR}/shaders")
set(SHADER_BIN_DIR "${CMAKE_BINARY_DIR}/shaders")

file(MAKE_DIRECTORY ${SHADER_BIN_DIR})

# Function to compile GLSL shaders
function(compile_shaders)
    file(GLOB SHADER_FILES "${SHADER_SRC_DIR}/*.vert" "${SHADER_SRC_DIR}/*.frag" "${SHADER_SRC_DIR}/*.comp")

    foreach(SHADER ${SHADER_FILES})
        get_filename_component(SHADER_NAME ${SHADER} NAME)
        set(SPIRV_OUTPUT "${SHADER_BIN_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT ${SPIRV_OUTPUT}
            COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${SHADER} -o ${SPIRV_OUTPUT}
            DEPENDS ${SHADER}
            COMMENT "Compiling ${SHADER_NAME} to SPIR-V"
            VERBATIM
        )

        list(APPEND SPIRV_BINARIES ${SPIRV_OUTPUT})
    endforeach()

    set(SPIRV_BINARIES ${SPIRV_BINARIES} PARENT_SCOPE)
endfunction()

# Call function to compile shaders
compile_shaders()

# Ensure shaders are built before the main executable
add_custom_target(Shaders DEPENDS ${SPIRV_BINARIES})


add_executable(VoxelEngine ${SOURCES})
target_link_libraries(VoxelEngine glfw Vulkan::Vulkan)
add_dependencies(${PROJECT_NAME} Shaders)
add_dependencies(VoxelEngine Shaders)
