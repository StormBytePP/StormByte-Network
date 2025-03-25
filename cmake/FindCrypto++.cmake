# FindCrypto++.cmake
# Locates the Crypto++ library and provides imported CMake targets.

# Defines:
# - Crypto++_FOUND: True if the library and headers are found
# - Crypto++_INCLUDE_DIRS: Path to Crypto++ include directory
# - Crypto++_LIBRARIES: Crypto++ library to link against
# - Crypto++_VERSION: Version of Crypto++ found (if available)

# Locate the Crypto++ include directory
find_path(Crypto++_INCLUDE_DIRS
          NAMES cryptlib.h sha256.h
		  PATH_SUFFIXES cryptopp
          PATHS ${CMAKE_PREFIX_PATH} /usr/include /usr/local/include
)

# Locate the Crypto++ library
find_library(Crypto++_LIBRARIES
             NAMES cryptopp
             PATH_SUFFIXES lib lib64
             PATHS ${CMAKE_PREFIX_PATH} /usr/lib /usr/lib64 /usr/local/lib /usr/local/lib64
)

# Check if we found both the headers and the library
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Crypto++ DEFAULT_MSG
    Crypto++_INCLUDE_DIRS
    Crypto++_LIBRARIES
)

# Provide an imported target if everything was found
if (Crypto++_FOUND)
    add_library(cryptopp UNKNOWN IMPORTED GLOBAL)
    set_target_properties(cryptopp PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${Crypto++_INCLUDE_DIRS}"
        IMPORTED_LOCATION "${Crypto++_LIBRARIES}"
    )
	message(STATUS "Found Crypto++: ${Crypto++_LIBRARIES}")
endif()