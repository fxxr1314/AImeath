cmake_minimum_required(VERSION 3.12)
project(demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(module/core)
add_subdirectory(module/app)
file(GLOB APP_DIRS CONFIGURE_DEPENDS desktop/app/*)
foreach(dir ${APP_DIRS})
  if(EXISTS ${dir}/backend/CMakeLists.txt)
    add_subdirectory(${dir}/backend)
  endif()
endforeach()
add_subdirectory(backend)

add_executable(demo main.cpp)
target_link_libraries(demo app core dl)
add_dependencies(demo snake gomoku pacman go)

set_target_properties(demo PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}/output/lib"
)
