include(GNUInstallDirs)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sources
file(GLOB_RECURSE STORMBYTE_CONFIG_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cxx")

# Library
add_library(StormByte-Network SHARED ${STORMBYTE_CONFIG_SOURCES})
add_library(StormByte::Network ALIAS StormByte-Network)
target_link_libraries(StormByte-Network PUBLIC StormByte StormByte-Buffer)
set_target_properties(StormByte-Network PROPERTIES
	LINKER_LANGUAGE CXX
	SOVERSION		${CMAKE_PROJECT_VERSION}
	VERSION 		${CMAKE_PROJECT_VERSION}
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
	target_link_options(StormByte-Network PRIVATE $<$<CONFIG:Release>:/LTCG /GL>)
else()
	set(CMAKE_CXX_FLAGS_DEBUG "-pipe -g -ggdb -Wall -Wextra -Wnon-virtual-dtor -pedantic -pedantic-errors -O0")
	target_compile_options(StormByte-Network PRIVATE -fvisibility=hidden $<$<COMPILE_LANGUAGE:CXX>:-fvisibility-inlines-hidden>)
endif()

# Include directories
target_include_directories(StormByte-Network
	SYSTEM BEFORE PUBLIC "${CMAKE_CURRENT_LIST_DIR}/public" "${CMAKE_CURRENT_LIST_DIR}/private"
)

# Install
if (NOT STORMBYTE_AS_DEPENDENCY)
	install(TARGETS StormByte-Network
		ARCHIVE 		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		LIBRARY 		DESTINATION "${CMAKE_INSTALL_LIBDIR}"
		RUNTIME 		DESTINATION ${CMAKE_INSTALL_BINDIR}
	)
	install(DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/public/StormByte/"
		DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/StormByte"
		FILES_MATCHING
		PATTERN "*.h"
		PATTERN "*.hxx"
	)
endif()
