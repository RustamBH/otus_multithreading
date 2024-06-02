cmake_minimum_required(VERSION 3.5)

project(33.homework)

add_executable(CountWords	count_words.cpp)
set_target_properties(CountWords PROPERTIES CXX_STANDARD 17 CXX_STANDARD_REQUIRED ON)