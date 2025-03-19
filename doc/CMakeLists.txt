option(ENABLE_DOC "Enable documentation generation with Doxygen" OFF)

if(ENABLE_DOC)
	find_package(Doxygen REQUIRED)

	# Doxygen basic settings
	set(DOXYGEN_PROJECT_NAME "${PROJECT_NAME}")
	set(DOXYGEN_PROJECT_NUMBER "${PROJECT_VERSION}")
	set(DOXYGEN_PROJECT_BRIEF "${PROJECT_DESCRIPTION}")
	set(DOXYGEN_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/docs")
	set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_SOURCE_DIR}/README.md")

	# Exclude .cxx files
	set(DOXYGEN_EXCLUDE_PATTERNS "*.cxx")

	# Doxygen output formats
	set(DOXYGEN_GENERATE_HTML YES)
	set(DOXYGEN_HTML_OUTPUT ".")
	set(DOXYGEN_GENERATE_MAN NO)
	set(DOXYGEN_GENERATE_LATEX NO)

	# Add documentation target
	doxygen_add_docs(
		StormByteNetworkDoc
		"${CMAKE_SOURCE_DIR}/lib/public/StormByte" # Input directory
		"${CMAKE_SOURCE_DIR}/README.md" # Explicitly include README.md
		COMMENT "Generate HTML pages"
	)
endif()