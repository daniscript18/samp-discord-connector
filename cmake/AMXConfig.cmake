# Compatibility shim for projects that expect the legacy AMXConfig module.
# The AMX sources in this tree rely on the old `LINUX` preprocessor macro to
# pull in the right platform headers and typedefs.
if(UNIX AND NOT APPLE)
	add_definitions(-DLINUX)
endif()

include_directories("${PROJECT_SOURCE_DIR}/src/amx")
