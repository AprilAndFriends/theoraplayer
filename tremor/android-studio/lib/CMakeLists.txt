cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(tremor)

include("../../../../../hltypes/android-studio/generic/CMakeLists.txt")

# defining manually so source files don't need to be changed
file(
	GLOB CppSrc
    "../../block.c"
    "../../codebook.c"
    "../../floor0.c"
    "../../floor1.c"
    "../../info.c"
    "../../mapping0.c"
    "../../mdct.c"
    "../../registry.c"
    "../../res012.c"
    "../../sharedbook.c"
    "../../synthesis.c"
    "../../vorbisfile.c"
    "../../window.c"
)

add_library(tremor STATIC ${CppSrc})

include_directories(AFTER "../..")
include_directories(AFTER "../../../ogg/include")

add_definitions(
	-Wno-unused-const-variable
	-Wno-unused-variable
)

if (DEFINED __arm__)
	add_definitions(
		-D_ARM_ASSEM_
	)
endif()

target_link_libraries(
	tremor
)
