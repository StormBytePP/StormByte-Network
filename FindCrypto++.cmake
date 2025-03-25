# FindCrypto++.cmake
find_path(CRYPTO++_INCLUDE_DIR cryptlib.h
  PATHS
    /usr/include
    /usr/local/include
    /opt/local/include
)

find_library(CRYPTO++_LIBRARIES NAMES cryptopp
  PATHS
    /usr/lib
    /usr/local/lib
    /opt/local/lib
)

if (CRYPTO++_INCLUDE_DIR AND CRYPTO++_LIBRARIES)
  set(CRYPTO++_FOUND TRUE)
  message(STATUS "Found Crypto++: ${CRYPTO++_INCLUDE_DIR}, ${CRYPTO++_LIBRARIES}")
else()
  set(CRYPTO++_FOUND FALSE)
  message(STATUS "Crypto++ not found.")
endif()

mark_as_advanced(CRYPTO++_INCLUDE_DIR CRYPTO++_LIBRARIES)
