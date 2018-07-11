cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

add_definitions(
	-D_ANDROID
	-D_LIB
	-fstack-protector
	-fstrict-aliasing
	-fomit-frame-pointer
	-no-canonical-prefixes
	-g3
	-Wall
	-Wno-parentheses
	-Wno-overloaded-virtual
	-Wstrict-overflow=0
)

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_DEBUG")

# these are for compatibility, will be removed later once the code base has been cleaned up
if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
	add_definitions(
		-D_ARM
	)
else()
	add_definitions(
		-D_X86
	)
endif()

include_directories(AFTER "../..")
include_directories(AFTER "../../src")
include_directories(AFTER "../../../include")

link_directories(
	"lib/android/${ANDROID_ABI}"
	"lib/lib/android/${ANDROID_ABI}"
	"../lib/android/${ANDROID_ABI}"
	"../lib/lib/android/${ANDROID_ABI}"
	"../../lib/android/${ANDROID_ABI}"
	"../../lib/lib/android/${ANDROID_ABI}"
	"../../../lib/android/${ANDROID_ABI}"
	"../../../lib/lib/android/${ANDROID_ABI}"
	"../../../../lib/android/${ANDROID_ABI}"
	"../../../../lib/lib/android/${ANDROID_ABI}"
)

find_library(libandroid_support android_support)
find_library(libunwind unwind)
find_library(liblog log)
find_library(libz z)
find_library(libc c)
find_library(libandroid android)
find_library(libGLESv2 GLESv2)
find_library(libOpenSLES OpenSLES)

set(SYSTEM_LIBS ${liblog} ${libz} ${libc} ${libandroid} ${libGLESv2})
