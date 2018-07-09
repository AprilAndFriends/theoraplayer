cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(dummy)

include("../../../theoraplayer/android-studio/generic/CMakeLists.txt")

file(
	GLOB_RECURSE CppSrc
	"../../demos/*.cpp"
)

add_library(dummy SHARED ${CppSrc})

include_directories(AFTER "../../demos")

add_subdirectory("../../../theoraplayer/android-studio/lib" theoraplayer)
add_subdirectory("../../../theora/android-studio/lib" theora)
add_subdirectory("../../../tremor/android-studio/lib" tremor)
add_subdirectory("../../../ogg/android-studio/lib" ogg)

target_link_libraries(
	dummy
	theoraplayer
	theora
	tremor
	ogg
	${liblog}
	${libz}
	${libc}
	${libdl}
	${libandroid}
	${libGLESv2}
)
