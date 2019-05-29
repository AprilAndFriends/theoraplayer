cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

add_definitions(
	-D_ANDROID
	-D_LIB
	-DAPP_UNIFIED_HEADERS
	-fstack-protector
	-fstrict-aliasing
	-fomit-frame-pointer
	-fintegrated-as
	-no-canonical-prefixes
	-Wall
	-Wno-parentheses
	-Wno-overloaded-virtual
	-Wstrict-overflow=0
)

# some additional flags
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_ASM_FLAGS_DEBUG "${CMAKE_ASM_FLAGS_DEBUG} -DDEBUG -D_DEBUG")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -DNDEBUG")

# forcing integrated assembler
string(REPLACE " -fno-integrated-as" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE " -fno-integrated-as" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE " -fno-integrated-as" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -fintegrated-as")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -fintegrated-as")
set(CMAKE_ASM_FLAGS_RELEASE "${CMAKE_ASM_FLAGS_RELEASE} -fintegrated-as")

# removing possibly problematic flags
string(REPLACE " -fstack-protector-strong" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
string(REPLACE " -fstack-protector-strong" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
string(REPLACE " -fstack-protector-strong" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
	add_definitions(
		-D_ARM
	)
	# disabling thumb mode
	string(REPLACE " -mthumb" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	string(REPLACE " -mthumb" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE " -mthumb" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")
else()
	add_definitions(
		-D_X86
	)
endif()

# some default includes
include_directories(AFTER "../..")
include_directories(AFTER "../../src")
include_directories(AFTER "../../../include")

# some default link directories
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

# some default libraries
find_library(liblog log)
find_library(libz z)
find_library(libc c)
find_library(libandroid android)
find_library(libGLESv2 GLESv2)
find_library(libOpenSLES OpenSLES)

set(SYSTEM_LIBS ${liblog} ${libz} ${libc} ${libandroid} ${libGLESv2})
