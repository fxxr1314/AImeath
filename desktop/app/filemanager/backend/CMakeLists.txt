cmake_minimum_required(VERSION 3.12)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(filemanager LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)

set(BOOST_ROOT "/usr/local/boost_1_91" CACHE PATH "Boost installation root directory")
find_package(Boost REQUIRED COMPONENTS json)

add_library(filemanager SHARED
    src/filemgr.cpp
)
target_include_directories(filemanager PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(filemanager PRIVATE ${CMAKE_SOURCE_DIR}/module/app/include)
target_link_libraries(filemanager PRIVATE Boost::json)
