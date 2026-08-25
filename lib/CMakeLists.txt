include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE STORMBYTE_NETWORK_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cxx")

add_library(StormByte-Network SHARED ${STORMBYTE_NETWORK_SOURCES})
add_library(StormByte::Network ALIAS StormByte-Network)

set_target_properties(StormByte-Network PROPERTIES
	LINKER_LANGUAGE CXX
	SOVERSION ${CMAKE_PROJECT_VERSION}
	VERSION ${CMAKE_PROJECT_VERSION}
	POSITION_INDEPENDENT_CODE ON
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON
)

if(WIN32)
	target_link_libraries(StormByte-Network PRIVATE ws2_32 iphlpapi)
endif()

if(MSVC)
	target_compile_options(StormByte-Network PRIVATE
		/EHsc
		$<$<CONFIG:Release>:/O2>
		$<$<CONFIG:Debug>:/Od>
	)
	target_compile_definitions(StormByte-Network PUBLIC UNICODE NOMINMAX)
else()
	target_compile_options(StormByte-Network PRIVATE
		$<$<CONFIG:Debug>:-pipe -g -ggdb -Wall -Wextra -Wnon-virtual-dtor -pedantic -pedantic-errors -O0>
	)
endif()

target_include_directories(StormByte-Network
	SYSTEM BEFORE PUBLIC
		"${CMAKE_CURRENT_LIST_DIR}/public"
		"${CMAKE_CURRENT_LIST_DIR}/private"
)
