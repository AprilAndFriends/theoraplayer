cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(ogg)

include("../../../theoraplayer/android-studio/generic/CMakeLists.txt")

# defining manually so source files don't need to be changed
file(
	GLOB CppSrc
	"../../src/bitwise.c"
	"../../src/framing.c"
)

add_library(ogg STATIC ${CppSrc})

include_directories(AFTER "../../include")

add_definitions(
	-DLIBOGG_EXPORTS
	-D_USRDLL
)

target_link_libraries(
	ogg
)
