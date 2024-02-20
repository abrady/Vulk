cmake_minimum_required(VERSION 3.10)

# Set the project name
project(VulkSamples)

# Add an executable
file(GLOB SOURCES "*.cpp")
add_executable(VulkRenderer ${SOURCES})
# Link the Vulk library to your executable
target_link_libraries(VulkRenderer Vulk)
