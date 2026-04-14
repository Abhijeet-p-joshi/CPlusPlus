cmake_minimum_required(VERSION 3.16)
project(CPlusPlusExamples)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable testing
enable_testing()

# Add subdirectories
add_subdirectory(Design_Patterns)
add_subdirectory(Multithreading)
add_subdirectory(Smart_Pointers)