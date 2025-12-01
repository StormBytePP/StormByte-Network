option(ENABLE_TEST "Enable Unit Tests" OFF)
if(ENABLE_TEST AND NOT STORMBYTE_AS_DEPENDENCY)
	# Combined server/client socket test (multi-client)
	# It is a private class, keep for internal testing only
	# add_executable(SocketMultiTest socket_multi_test.cxx)
	# target_link_libraries(SocketMultiTest StormByte::Network)
	# add_test(NAME SocketMultiTest COMMAND SocketMultiTest)
endif()