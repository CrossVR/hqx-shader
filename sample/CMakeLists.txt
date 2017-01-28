cmake_minimum_required(VERSION 3.5.0)
project (hqx-sample)

add_subdirectory (glfw)
include_directories(BEFORE "${GLFW_SOURCE_DIR}/include")
include_directories(BEFORE "${GLFW_SOURCE_DIR}/deps")
set(GLAD "${GLFW_SOURCE_DIR}/deps/glad/glad.h" "${GLFW_SOURCE_DIR}/deps/glad.c")
link_libraries(glfw)

add_definitions(-DLODEPNG_NO_COMPILE_CPP)
set(LODEPNG lodepng.h lodepng.c)

add_executable (hqx-sample WIN32 main.c ${GLAD} ${LODEPNG})

if (MSVC)
    # Tell MSVC to use main instead of WinMain for Windows subsystem executables
    set_target_properties(hqx-sample PROPERTIES LINK_FLAGS "/ENTRY:mainCRTStartup")
endif()
