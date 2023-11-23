cmake_minimum_required(VERSION 3.21)
# Installs vcpkg locally into this project if no valid vcpkg found
include(cmake/vcpkg.cmake)

project(fairyforest VERSION 0.0.1)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME}
    "src/main.cpp"
    "src/application.cpp"
    "src/window.cpp"
    "src/context.cpp"
    "src/backend/device.cpp"
    "src/backend/instance.cpp"
)

find_package(fmt CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)
target_link_libraries(${PROJECT_NAME} PRIVATE
    Vulkan::Vulkan
    fmt::fmt
    glfw
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_link_libraries(${PROJECT_NAME} PRIVATE Dwmapi)
endif()

# Debug mode defines
target_compile_definitions(${PROJECT_NAME} PRIVATE "$<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:BACKEND_LOGGING>")