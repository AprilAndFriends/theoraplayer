cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(aprilvideo)

include("../../../hltypes/android-studio/generic/CMakeLists.txt")

file(
	GLOB_RECURSE CppSrc
	"../../src/*.c"
	"../../src/*.cpp"
)

add_library(aprilvideo STATIC ${CppSrc})

include_directories(AFTER "../../include/aprilvideo")
include_directories(AFTER "../../src")
include_directories(AFTER "../../../theoraplayer/include")
include_directories(AFTER "../../../xal/include")
include_directories(AFTER "../../../aprilui/include")
include_directories(AFTER "../../../april/include")
include_directories(AFTER "../../../hltypes/include")
include_directories(AFTER "../../../gtypes/include")

add_definitions(
	-DAPRILVIDEO_EXPORTS
)

target_link_libraries(
	aprilvideo
)
