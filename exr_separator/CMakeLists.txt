# CMakeList.txt : CMake project for exr_separator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.


set (CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if(MSVC)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
endif(MSVC)

find_package(OpenImageIO)

include_directories(${OIIO_INCLUDE_DIRS})

add_executable (exr_separator "exr_separator.cpp")

target_link_libraries(${PROJECT_NAME} ${OIIO_LIBRARIES})




# TODO: Add tests and install targets if needed.
