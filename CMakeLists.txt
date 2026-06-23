cmake_minimum_required(VERSION 3.12)
project(demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(module/game)
add_subdirectory(module/core)
file(GLOB APP_DIRS CONFIGURE_DEPENDS desktop/app/*)
foreach(dir ${APP_DIRS})
  if(EXISTS ${dir}/backend/CMakeLists.txt)
    add_subdirectory(${dir}/backend)
  endif()
endforeach()
add_subdirectory(backend)

add_executable(demo main.cpp)
target_include_directories(demo PRIVATE module/game/include module/core/include)
target_link_libraries(demo game core dl)
add_dependencies(demo game snake gomoku pacman go)

# 让运行 demo 时 dlopen 能找到 .so
set_target_properties(demo PROPERTIES
    BUILD_RPATH "${CMAKE_BINARY_DIR}/output/lib"
)
