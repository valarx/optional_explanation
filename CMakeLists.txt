cmake_minimum_required(VERSION 3.10)
project(optional_explanation)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.70 REQUIRED)

add_executable(optional_explanation "")

target_sources(optional_explanation 
    PRIVATE
        main.cpp)

target_link_libraries(optional_explanation
    PRIVATE
        Boost::boost)

target_compile_options(optional_explanation PRIVATE -Wall -Wextra -Wpedantic -Werror)
target_compile_features(optional_explanation PRIVATE cxx_std_17)