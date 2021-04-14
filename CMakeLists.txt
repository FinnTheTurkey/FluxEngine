cmake_minimum_required(VERSION 3.0.0)
project(FluxEngine VERSION 0.1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set( CMAKE_CXX_STANDARD 17 )

include(CTest)
enable_testing()

# Enable hell
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wpedantic") # -Wextra -Weverything")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fsanitize=bounds")

if (FLUX_GCC_PROFILE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg -fstack-protector-strong")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pg")
endif()

if (CMAKE_BUILD_TYPE MATCHES "Release")
    # Remove assers for release builds
    add_compile_definitions(FLUX_NO_ASSERT)

    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        # Enable fast math
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -ffast-math")
    endif()
endif()

option(FLUX_NO_CPU_FREE "If enabled, Flux Engine won't free vertex data and textures on the cpu once it's been sent to the gpu" OFF)

if (FLUX_NO_CPU_FREE)
    add_compile_definitions(FLUX_NO_CPU_FREE)
endif()

add_compile_definitions(FLUX_NO_THREADING)

# Make sure GLM always creates matricies with values that actually work
add_compile_definitions(GLM_FORCE_CTOR_INIT)

if (NOT DEFINED FLUX_BUILD_GLFW)
    set(FLUX_BUILD_GLFW ON)
    # message("Building GLFW")
endif()

if (EMSCRIPTEN)
    set(FLUX_INCLUDE_GLFW OFF)
endif()

if (FLUX_BUILD_GLFW)
    if (NOT DEFINED FLUX_INCLUDE_GLFW)
        set(FLUX_INCLUDE_GLFW ON)
    endif()
endif()
# if (NOT DEFINED FLUX_USE_GLAD)
#     set(FLUX_USE_GLAD ON)
# endif()

if (FLUX_BUILD_GLFW)
    set(GLFW_SOURCE Src/OpenGL/GLFW/GLFW.cc)
endif()

add_library(FluxEngine STATIC
    # Headers
    # ===================
    Include/Flux/Flux.hh
    Include/Flux/ECS.hh
    Include/Flux/Log.hh
    Include/Flux/Debug.hh
    # Include/Flux/Threads.hh
    Include/Flux/Resources.hh
    Include/Flux/Input.hh

    # Renderer headers
    Include/Flux/Renderer.hh
    Include/Flux/OpenGL/GLRenderer.hh

    # Source files
    # ===================
    Src/ECS.cc 
    Src/Entry.cc
    Src/Threads.cc
    Src/DevTools/Log.cc
    Src/DevTools/Debug.cc
    Src/Res/Resources.cc
    Src/Res/Serialize.cc

    # Renderer source files
    Src/Renderer/Renderer.cc
    Src/Renderer/Transform.cc
    Src/OpenGL/GLRenderer.cc

    # Physics
    Src/Physics/Physics.cc
    Src/Physics/RigidBody.cc

    # Source files for GLFW window
    ${GLFW_SOURCE}
)

target_include_directories(FluxEngine PUBLIC Include)

# STB
target_include_directories(FluxEngine PUBLIC ThirdParty/Include)

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
if (NOT EMSCRIPTEN)
    set(OpenGL_GL_PREFERENCE GLVND)
    find_package(OpenGL REQUIRED)
    target_link_libraries(FluxEngine PUBLIC OpenGL::GL)
endif()

# if (FLUX_USE_GLAD)
# Glad
add_subdirectory(ThirdParty/glad)
target_link_libraries(FluxEngine PUBLIC glad)
target_link_libraries(FluxEngine PUBLIC ${CMAKE_DL_LIBS})
# else()
#     add_compile_definitions(FLUX_NO_GLAD)
# endif()

if (FLUX_INCLUDE_GLFW)
    # GLFW
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

    add_subdirectory(ThirdParty/glfw)
    target_link_libraries(FluxEngine PUBLIC glfw)
endif()

# GLM
# add_subdirectory(ThirdParty/glm)
# target_link_libraries(FluxEngine PUBLIC glm)
target_include_directories(FluxEngine PUBLIC ThirdParty/glm)

# FluxArc
# Add FluxARC
add_subdirectory(FluxTools/FluxArc)
target_link_libraries(FluxEngine PUBLIC FluxArc)