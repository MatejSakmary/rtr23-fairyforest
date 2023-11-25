cmake_minimum_required(VERSION 3.21)
# Installs vcpkg locally into this project if no valid vcpkg found
include(cmake/vcpkg.cmake)

project(fairyforest VERSION 0.0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Shader offline compilation
macro(compile_glsl GLSL_SOURCES STAGE)
	foreach(GLSL ${GLSL_SOURCES})
  		get_filename_component(FILE_NAME ${GLSL} NAME)
  		set(SPIRV "src/shaders/bin/${FILE_NAME}.spv")
		add_custom_command(
			OUTPUT ${SPIRV}
			COMMAND ${CMAKE_COMMAND} -E make_directory "src/shaders/bin/"
			COMMAND ${GLSLC} -fshader-stage=${STAGE} ${GLSL} -I. -o ${SPIRV}
			WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
			DEPENDS ${GLSL}
		)
  		list(APPEND SPIRV_BINARY_FILES ${SPIRV})
	endforeach(GLSL)
endmacro()

find_program(GLSLC "glslc")
if(NOT GLSLC)
	message(FATAL_ERROR "glslc not found!")
endif()

set (GLSL_VERT_SOURCE_FILES
    "src/shaders/triangle.vert"
)

set (GLSL_FRAG_SOURCE_FILES
    "src/shaders/triangle.frag"
)

compile_glsl("${GLSL_VERT_SOURCE_FILES}" "vert")
compile_glsl("${GLSL_FRAG_SOURCE_FILES}" "frag")

add_custom_command(
    OUTPUT shaders.txt 
    COMMAND ${CMAKE_COMMAND} -E touch "shaders.txt"
    DEPENDS ${SPIRV_BINARY_FILES}
)

add_executable(${PROJECT_NAME}
    "src/main.cpp"
    "src/application.cpp"
    "src/window.cpp"
    "src/context.cpp"
    "src/backend/device.cpp"
    "src/backend/instance.cpp"
    "src/backend/features.cpp"
    "src/backend/swapchain.cpp"
    "src/backend/gpu_resource_table.cpp"
    "src/backend/command_buffer.cpp"
    "src/backend/pipeline.cpp"
    "src/rendering/renderer.cpp"
    "shaders.txt"
)

# add_dependencies(${PROJECT_NAME} shaders)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(Vulkan REQUIRED)


target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    GPUOpen::VulkanMemoryAllocator
    fmt::fmt
    glfw
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} PRIVATE Dwmapi)
endif()

# Debug mode defines
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:BACKEND_LOGGING>")