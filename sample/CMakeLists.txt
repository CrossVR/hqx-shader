cmake_minimum_required(VERSION 3.5.0)
project (hqx-sample)

add_subdirectory (glfw)
include_directories(BEFORE "${GLFW_SOURCE_DIR}/include")
include_directories(BEFORE "${GLFW_SOURCE_DIR}/deps")
set(GLAD "${GLFW_SOURCE_DIR}/deps/glad/glad.h" "${GLFW_SOURCE_DIR}/deps/glad.c")
link_libraries(glfw)

set(LODEPNG lodepng/lodepng.h lodepng/lodepng.cpp)
include_directories(BEFORE lodepng)

add_executable (hqx-sample WIN32 main.cpp ${GLAD} ${LODEPNG})

if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(hqx-sample PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()
