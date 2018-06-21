cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)

project(theora)

include("../../../hltypes/android-studio/generic/cmake-preprocessors.txt")

# defining manually so source files don't need to be changed
file(
	GLOB CppSrc
    "../../lib/collect.c"
    "../../lib/fdct.c"
    "../../lib/huffenc.c"
    "../../lib/mathops.c"
    "../../lib/rate.c"
    "../../lib/tokenize.c"
    "../../lib/apiwrapper.c"
    "../../lib/bitpack.c"
    "../../lib/decapiwrapper.c"
    "../../lib/decinfo.c"
    "../../lib/decode.c"
    "../../lib/dequant.c"
    "../../lib/fragment.c"
    "../../lib/huffdec.c"
    "../../lib/idct.c"
    "../../lib/info.c"
    "../../lib/internal.c"
    "../../lib/quant.c"
    "../../lib/state.c"
)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
	file(
		GLOB CppSrcPlatform
		"../../lib/arm_llvm_android/armcpu.c"
		"../../lib/arm_llvm_android/armstate.c"
		"../../lib/arm_llvm_android/armbits.asm"
		"../../lib/arm_llvm_android/armfrag.asm"
		"../../lib/arm_llvm_android/armidct.asm"
		"../../lib/arm_llvm_android/armloop.asm"
	)
else()
	file(
		GLOB CppSrcPlatform
		"../../lib/x86/mmxfdct.c"
		"../../lib/x86/mmxfrag.c"
		"../../lib/x86/mmxidct.c"
		"../../lib/x86/mmxstate.c"
		"../../lib/x86/sse2fdct.c"
		"../../lib/x86/sse2idct.c"
		"../../lib/x86/x86cpu.c"
		"../../lib/x86/x86state.c"
	)
endif()

add_library(theora STATIC ${CppSrc} ${CppSrcPlatform})

include_directories(AFTER "../../include/theora")
include_directories(AFTER "../../lib")
include_directories(AFTER "../../lib/arm_llvm_android")
include_directories(AFTER "../../../xal/lib/vorbis/include")
include_directories(AFTER "../../../vorbis/include")
include_directories(AFTER "../../../xal/lib/ogg/include")
include_directories(AFTER "../../../ogg/include")

add_definitions(
	-DTHEORA_EXPORTS
)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
	add_definitions(
		-DOC_ARM_ASM
		-DOC_ARM_ASM_EDSP=1
		-DOC_ARM_ASM_MEDIA=1
		-DOC_ARM_ASM_NEON=1
		-DOC_ARM_CAN_UNALIGN=1
		-DOC_ARM_CAN_UNALIGN_LDRD=0
		-mfpu=neon
	)
else()
	add_definitions(
		-DOC_X86_ASM
	)
endif()

target_link_libraries(
	theora
)
