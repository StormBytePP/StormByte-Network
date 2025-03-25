option(ENABLE_TEST "Enable Unit Tests" OFF)
if(ENABLE_TEST AND NOT STORMBYTE_AS_DEPENDENCY)
	add_executable(AESTests aes_test.cxx)
	target_link_libraries(AESTests StormByte::Network)
	add_test(NAME AESTests COMMAND AESTests)

	add_executable(Bzip2Tests bzip2_test.cxx)
	target_link_libraries(Bzip2Tests StormByte::Network)
	add_test(NAME Bzip2Tests COMMAND Bzip2Tests)

	add_executable(ECCTests ecc_test.cxx)
	target_link_libraries(ECCTests StormByte::Network)
	add_test(NAME ECCTests COMMAND ECCTests)

	add_executable(GzipTests gzip_test.cxx)
	target_link_libraries(GzipTests StormByte::Network)
	add_test(NAME GzipTests COMMAND GzipTests)

	add_executable(SHA256Tests sha256_test.cxx)
	target_link_libraries(SHA256Tests StormByte::Network)
	add_test(NAME SHA256Tests COMMAND SHA256Tests)

	add_executable(SocketTests socket_test.cxx)
	target_link_libraries(SocketTests StormByte::Network)
	add_test(NAME SocketTests COMMAND SocketTests)
	if (GITHUB_WORKFLOW)
		target_compile_definitions(SocketTests PRIVATE GITHUB_WORKFLOW)
	endif()

	add_executable(RSATests rsa_test.cxx)
	target_link_libraries(RSATests StormByte::Network)
	add_test(NAME RSATests COMMAND RSATests)
endif()