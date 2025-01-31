cmake_minimum_required(VERSION 3.8)
project(SharedMemoryExample)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)

# Manually set Python 3.8.20 executable path (update this path if necessary)
set(PYTHON_EXECUTABLE /home/ufg/anaconda3/envs/fa_5glena/bin/python)

# Find Python3 using the explicitly specified Python executable
find_package(Python3 3.8 EXACT REQUIRED COMPONENTS Interpreter Development)

# Debugging: Print Python paths and versions for confirmation
message(STATUS "Using Python Executable: ${Python3_EXECUTABLE}")
message(STATUS "Detected Python3 Version: ${Python3_VERSION}")

# Ensure the correct Python version is being used
if(NOT ${Python3_VERSION} VERSION_EQUAL "3.8.20")
    message(FATAL_ERROR "Incorrect Python version detected: ${Python3_VERSION}. Expected version: 3.8.20.")
endif()

# Find pybind11 (make sure it's installed and accessible)
find_package(pybind11 REQUIRED)

# Add the shared library (using the correct source file)
add_library(shared_memory_writer MODULE shared_memory_writer.cc)

# Ensure proper naming of the Python module (.so file)
set_target_properties(shared_memory_writer PROPERTIES PREFIX "" SUFFIX ".so")

# Link pybind11 and Python
target_link_libraries(shared_memory_writer PRIVATE pybind11::module Python3::Python)

# Optional: print paths for debugging
message(STATUS "Python3 Development Libraries: ${Python3_LIBRARIES}")
message(STATUS "Python3 Include Dir: ${Python3_INCLUDE_DIRS}")

