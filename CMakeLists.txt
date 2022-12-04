cmake_minimum_required(VERSION 3.22)

project(my_antivirus)

find_package(Boost 1.74 COMPONENTS program_options system REQUIRED)
find_package(fmt REQUIRED)

file (GLOB ${PROJECT_NAME}_SOURCES src/database.cpp src/file_list.cpp src/list.cpp src/file.cpp src/process.cpp src/utils.cpp src/tables/sqlite3.c src/tables/*.cpp)


add_executable(main ${${PROJECT_NAME}_SOURCES} src/main.cpp)
add_executable(lists ${${PROJECT_NAME}_SOURCES} src/lists.cpp)

target_link_libraries(main ${Boost_LIBRARIES} fmt::fmt)
target_link_libraries(lists ${Boost_LIBRARIES} fmt::fmt)

target_include_directories(main PRIVATE include/ ${Boost_INCLUDE_DIR})
target_include_directories(lists PRIVATE include/ ${Boost_INCLUDE_DIR})

target_compile_features(main PUBLIC cxx_std_20)
target_compile_features(lists PUBLIC cxx_std_20)

set_target_properties(main lists PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")
