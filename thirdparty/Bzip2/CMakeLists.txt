option(WITH_SYSTEM_BZIP2 "Use the system BZip2 library" ON)
if (WITH_SYSTEM_BZIP2)
	find_package (BZip2 REQUIRED)
else()
	add_subdirectory(src)
	# Set interface include directories for libz2
	target_include_directories(bz2 INTERFACE $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>)
	set_target_properties(bz2 PROPERTIES OUTPUT_NAME bz2)

endif()