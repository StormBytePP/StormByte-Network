if(MSVC)
	set(STORMBYTE_BUFFER_OPTSTR "TOOLCHAIN=clang-cl;ALIAS=StormByte::Buffer")
else()
	set(STORMBYTE_BUFFER_OPTSTR "ALIAS=StormByte::Buffer")
endif()

buildmaster_component(
	StormByte-Buffer
	"StormByte Suite - Buffer"
	"${CMAKE_CURRENT_LIST_DIR}/src"
	"WITH_STORMBYTE=BUNDLED"
	shared
	"StormByte-Buffer"
	"${STORMBYTE_BUFFER_OPTSTR}"
)
