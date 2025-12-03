option(ENABLE_ASAN "Enable AddressSanitizer (and UBSan) for debug builds" OFF)

if(ENABLE_ASAN AND NOT STORMBYTE_AS_DEPENDENCY AND NOT WIN32)
	# Recommend Debug builds for ASan
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)

	# GCC/Clang sanitizers
	set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}" CACHE STRING "C++ flags" FORCE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}" CACHE STRING "C flags" FORCE)

	# Improve ASan/UBSan diagnostics
	add_compile_options(-g)
	# Linker flags for sanitizers
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}" CACHE STRING "Executable linker flags" FORCE)
	set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}" CACHE STRING "Shared linker flags" FORCE)

	# Set global environment for all ctest runs.
	set_property(GLOBAL PROPERTY ENVIRONMENT 
		"ASAN_OPTIONS=detect_leaks=1:abort_on_error=1:strict_string_checks=1:alloc_dealloc_mismatch=1;UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1"
	)

	# Inform about ASan being enabled
	message(STATUS "AddressSanitizer and UndefinedBehaviorSanitizer enabled")
endif()