# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.
#
cmake_minimum_required (VERSION 3.8)

project ("exr_separator")
option(OIIO_LOCATION "Where is OpenImage")
# Include sub-projects.
add_subdirectory ("exr_separator")



