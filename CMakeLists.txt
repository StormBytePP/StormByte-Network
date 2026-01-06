cmake_minimum_required(VERSION 3.12)
set(CMAKE_DISABLE_IN_SOURCE_BUILD ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project("StormByte C++ Library: Network module"
	VERSION 0.0.9999
	DESCRIPTION "StormByte-Network is a StormByte library module for handling and create network connections"
	HOMEPAGE_URL "https://dev.stormbyte.org/StormByte-Network"
)

include(cmake/optimizations.cmake)
include(cmake/asan.cmake)
cmake_policy(SET CMP0079 NEW)

add_subdirectory(doc)
add_subdirectory(lib)
add_subdirectory(thirdparty)
add_subdirectory(test)

include(cmake/outputflags.cmake)
include(cmake/install.cmake)