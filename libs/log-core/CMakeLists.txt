cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)
project(samp-log-core VERSION 0.4)

if (NOT LOGCORE_VERSION)
	set(LOGCORE_VERSION ${PROJECT_VERSION})
endif()

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/libs/cmake")

set(CMAKE_CXX_STANDARD 11)

if (MSVC)
	add_link_options(/INCREMENTAL:NO)
	set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
elseif(UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
	set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

	set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
	add_link_options(
		"-static-libgcc"
		"-static-libstdc++"
	)
endif()

set(LOGCORE_LIBS_DIR "${PROJECT_SOURCE_DIR}/libs")

add_subdirectory(src)
