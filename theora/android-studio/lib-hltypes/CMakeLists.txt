cmake_minimum_required(VERSION 3.4.1)
set(CMAKE_VERBOSE_MAKEFILE on)
enable_language(ASM)

project(theora)

include("../../../hltypes/android-studio/generic/CMakeLists.txt")

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

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
	file(
		GLOB CppSrcPlatform
		"../../lib/arm_android_studio/armcpu.c"
		"../../lib/arm_android_studio/armstate.c"
		#"../../lib/arm_android_studio/armbits.asm"
		#"../../lib/arm_android_studio/armfrag.asm"
		#"../../lib/arm_android_studio/armidct.asm"
		#"../../lib/arm_android_studio/armloop.asm"
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

include_directories(AFTER "../../include")
include_directories(AFTER "../../lib")
include_directories(AFTER "../../lib/arm_llvm_android")
include_directories(AFTER "../../../xal/lib") # tremor
include_directories(AFTER "../../../xal/lib/ogg/include")

add_definitions(
	-DTHEORA_EXPORTS
	-Wno-shift-negative-value
)

if (${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm" OR ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
	string(REPLACE " -mfpu=vfpv3-d16" "" CMAKE_ASM_FLAGS "${CMAKE_ASM_FLAGS}")
	add_definitions(
		-mfpu=neon
	)
	if (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
		add_definitions(
			-DOC_ARM_ASM
			-DOC_ARM_ASM_EDSP=1
			-DOC_ARM_ASM_MEDIA=1
			-DOC_ARM_ASM_NEON=1
			-DOC_ARM_CAN_UNALIGN=1
			-DOC_ARM_CAN_UNALIGN_LDRD=0
		)
	endif()
elseif (NOT ${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
	add_definitions(
		-DOC_X86_ASM
	)
endif()

target_link_libraries(
	theora
)
