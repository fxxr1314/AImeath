cmake_minimum_required(VERSION 3.12)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(go LANGUAGES CXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/lib)

add_library(go SHARED
    src/board.cpp
    src/game.cpp
    ${CMAKE_SOURCE_DIR}/desktop/app/shared/game/src/game_app_adapter.cpp
)
target_include_directories(go PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(go PRIVATE ${CMAKE_SOURCE_DIR}/module/app/include)
target_include_directories(go PRIVATE ${CMAKE_SOURCE_DIR}/desktop/app/shared/game/include)

find_package(Boost REQUIRED COMPONENTS json)
target_link_libraries(go PRIVATE Boost::json)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
find_package(GTest REQUIRED)
add_executable(tests
    test/go_test.cpp
)
set_target_properties(tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/test)
target_link_libraries(tests go GTest::GTest)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../../../shared/game/include)

find_package(benchmark REQUIRED)
add_executable(bench_go benchmark/go_bench.cpp)
set_target_properties(bench_go PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/output/bench)
target_link_libraries(bench_go go benchmark::benchmark)
target_include_directories(bench_go PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
endif()
