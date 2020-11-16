cmake_minimum_required(VERSION 3.0.0)
project(FluxEngine VERSION 0.1.0)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CTest)
enable_testing()

add_library(FluxEngine 
# Headers
Include/Flux/Flux.hh
Include/Flux/ECS.hh
Include/Flux/Log.hh
Include/Flux/Threads.hh
# Source files
Src/ECS.cc 
Src/Entry.cc
Src/Threads.cc
Src/DevTools/Log.cc)

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