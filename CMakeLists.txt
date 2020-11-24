cmake_minimum_required(VERSION 3.0.0)
project(FluxEngine VERSION 0.1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic -Wextra -Weverything")

if (FLUX_GCC_PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

add_compile_definitions(FLUX_NO_THREADING)

add_library(FluxEngine 
# Headers
# ===================
Include/Flux/Flux.hh
Include/Flux/ECS.hh
Include/Flux/Log.hh
Include/Flux/Threads.hh
Include/Flux/Resources.hh

# Renderer headers
Include/Flux/Renderer.hh
Include/Flux/OpenGL/GLRenderer.hh

# Source files
# ===================
Src/ECS.cc 
Src/Entry.cc
Src/Threads.cc
Src/DevTools/Log.cc
Src/Res/Resources.cc

# Renderer source files
Src/Renderer/Renderer.cc
Src/Renderer/Transform.cc
Src/OpenGL/GLRenderer.cc
)

target_include_directories(FluxEngine PUBLIC Include)
# include_directories(Include)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(FluxEngine PRIVATE Threads::Threads)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)

# External depenencies
add_subdirectory(ThirdParty/term-colors)
target_link_libraries(FluxEngine PUBLIC termcolors)

# Open GL
set(OpenGL_GL_PREFERENCE GLVND)
find_package(OpenGL REQUIRED)
target_link_libraries(FluxEngine PUBLIC OpenGL::GL)

# Glad
add_subdirectory(ThirdParty/glad)
target_link_libraries(FluxEngine PUBLIC glad)

# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# GLM
add_subdirectory(ThirdParty/glm)
target_link_libraries(FluxEngine PUBLIC glm)

add_subdirectory(ThirdParty/glfw)
target_link_libraries(FluxEngine PUBLIC glfw)