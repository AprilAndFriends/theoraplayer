cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(theoraplayer)

include("../generic/CMakeLists.txt")

file(
	GLOB_RECURSE CppSrc
	"../../src/*.c"
	"../../src/*.cc"
	"../../src/*.cpp"
)

add_library(theoraplayer STATIC ${CppSrc})

include_directories(AFTER "../../include/theoraplayer")
include_directories(AFTER "../../src/formats")
include_directories(AFTER "../../src/YUV")
include_directories(AFTER "../../src/YUV/C")
include_directories(AFTER "../../src/YUV/libyuv")
include_directories(AFTER "../../src/YUV/libyuv/include")
include_directories(AFTER "../../src/YUV/android")
include_directories(AFTER "../../../theora/include")
include_directories(AFTER "../../..") # tremor
include_directories(AFTER "../../../ogg/include")

add_definitions(
	-DTHEORAPLAYER_EXPORTS
	-D_YUV_LIBYUV
	-D_USE_THEORA
	-D_USE_TREMOR
)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")
	if (${CMAKE_SIZEOF_VOID_P} EQUAL 8)
		add_definitions(
			# -mfpu=neon
		)
	else()
		add_definitions(
			-DLIBYUV_NEON
			-D__ARM_NEON__
			-mfpu=neon
		)
	endif()
else()
	add_definitions(
		-D__SSE2__
	)
endif()

target_link_libraries(
	theoraplayer
)
