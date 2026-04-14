function(add_samp_plugin target_name)
	set(_sources ${ARGN})

	if(NOT WIN32)
		list(FILTER _sources EXCLUDE REGEX "\\.def$")
	endif()

	if(WIN32)
		add_library(${target_name} SHARED ${_sources})
	else()
		add_library(${target_name} MODULE ${_sources})
	endif()

	set_target_properties(${target_name} PROPERTIES
		PREFIX ""
		OUTPUT_NAME ${target_name}
	)
endfunction()
