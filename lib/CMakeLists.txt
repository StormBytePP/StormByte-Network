include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sources
file(GLOB_RECURSE STORMBYTE_CONFIG_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cxx")

# Library
add_library(StormByte-Network SHARED ${STORMBYTE_CONFIG_SOURCES})
add_library(StormByte::Network ALIAS StormByte-Network)
set_target_properties(StormByte-Network PROPERTIES
	LINKER_LANGUAGE CXX
	SOVERSION		${CMAKE_PROJECT_VERSION}
	VERSION 		${CMAKE_PROJECT_VERSION}
)
set_target_properties(StormByte-Network PROPERTIES
	POSITION_INDEPENDENT_CODE        ON
	CXX_VISIBILITY_PRESET            hidden
	VISIBILITY_INLINES_HIDDEN        ON
)
# Link against the Winsock library (for Windows)
if(WIN32)
	target_link_libraries(StormByte-Network PRIVATE ws2_32.dll iphlpapi.dll)
endif()

# Compile options
if(MSVC)
	target_compile_options(StormByte-Network PRIVATE /EHsc)
	target_compile_options(StormByte-Network PRIVATE $<$<CONFIG:Release>:/O2>)
	target_compile_options(StormByte-Network PRIVATE $<$<CONFIG:Debug>:/Od>)
	target_compile_definitions(StormByte-Network PUBLIC UNICODE NOMINMAX) # Create problems if disabled
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-pipe -g -ggdb -Wall -Wextra -Wnon-virtual-dtor -pedantic -pedantic-errors -O0")
endif()

# Include directories
target_include_directories(StormByte-Network
	SYSTEM BEFORE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public" "${CMAKE_CURRENT_LIST_DIR}/private"
)

