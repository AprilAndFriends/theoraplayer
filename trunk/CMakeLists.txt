# CMakeLists.txt for building static libtheoraplayer library for 64-bit Linux
# authors: Joe LeVeque & Krešimir Špes
#

# This line indicates to Cmake that minimum version 2.8 is required.
cmake_minimum_required(VERSION 2.8 FATAL_ERROR)

if(UNIX)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # Force a 32-bit build on 64-bit machines for now.
 #   set( CMAKE_CXX_FLAGS "-m32" ) # enable this if you want a 32 bit build
 #   set( CMAKE_EXE_LINKER_FLAGS "-m32" )
  endif()
endif()

# Definitions for all libs
#add_definitions(-D_DEBUG)

# libogg

add_definitions(-DLIBOGG_EXPORTS)

include_directories(
   "ogg/include")
   
aux_source_directory(ogg/src OGG_SRC_DIR)

add_library (ogg STATIC ${OGG_SRC_DIR})

remove_definitions(-DLIBOGG_EXPORTS)

# libtheora

add_definitions(-DLIBTHEORA_EXPORTS -DOC_X86_ASM)

include_directories(
   "ogg/include"
   "theora/include")
   
list(APPEND THEORA_SOURCES
   theora/lib/analyze.c
   theora/lib/apiwrapper.c
   theora/lib/bitpack.c
   theora/lib/collect.c
   theora/lib/decapiwrapper.c
   theora/lib/decinfo.c
   theora/lib/decode.c
   theora/lib/dequant.c
   theora/lib/encapiwrapper.c
   theora/lib/encfrag.c
   theora/lib/encinfo.c
   theora/lib/encode.c
   theora/lib/enquant.c
   theora/lib/fdct.c
   theora/lib/fragment.c
   theora/lib/huffdec.c
   theora/lib/huffenc.c
   theora/lib/idct.c
   theora/lib/info.c
   theora/lib/internal.c
   theora/lib/mathops.c
   theora/lib/mcenc.c
   theora/lib/quant.c
   theora/lib/rate.c
   theora/lib/state.c
   theora/lib/tokenize.c
   theora/lib/x86/mmxencfrag.c
   theora/lib/x86/mmxfdct.c
   theora/lib/x86/mmxfrag.c
   theora/lib/x86/mmxidct.c
   theora/lib/x86/mmxstate.c
   theora/lib/x86/sse2idct.c
   theora/lib/x86/x86cpu.c
   theora/lib/x86/x86enquant.c
   theora/lib/x86/x86state.c)

add_library (theora STATIC ${THEORA_SOURCES})

remove_definitions(-DLIBTHEORA_EXPORTS)

# libvorbis

add_definitions(-DLIBVORBIS_EXPORTS)

include_directories(
   "ogg/include"
   "vorbis/lib"
   "vorbis/include")

list(APPEND VORBIS_SOURCES
   vorbis/lib/analysis.c
   vorbis/lib/bitrate.c
   vorbis/lib/block.c
   vorbis/lib/codebook.c
   vorbis/lib/envelope.c
   vorbis/lib/floor0.c
   vorbis/lib/floor1.c
   vorbis/lib/info.c
   vorbis/lib/lookup.c
   vorbis/lib/lpc.c
   vorbis/lib/lsp.c
   vorbis/lib/mapping0.c
   vorbis/lib/mdct.c
   vorbis/lib/psy.c
   vorbis/lib/registry.c
   vorbis/lib/res0.c
   vorbis/lib/sharedbook.c
   vorbis/lib/smallft.c
   vorbis/lib/synthesis.c
   vorbis/lib/vorbisenc.c
   vorbis/lib/window.c)

add_library (vorbis STATIC ${VORBIS_SOURCES})

remove_definitions(-DLIBVORBIS_EXPORTS)

# libtheoraplayer

add_definitions(-D__THEORA -D_YUV_LIBYUV -D__SSSE3__ -DTHEORAVIDEO_EXPORTS -D_LIB -D_LINUX)

include_directories(
   "ogg/include"
   "theora/include"
   "vorbis/include"
   "theoraplayer/include/theoraplayer"
   "theoraplayer/src/Theora/"
   "theoraplayer/src/YUV/"
   "theoraplayer/src/YUV/libyuv/include")
   
aux_source_directory(theoraplayer/src THEORAPLAYER_SRC_DIR1)
aux_source_directory(theoraplayer/src/Theora THEORAPLAYER_SRC_DIR2)
aux_source_directory(theoraplayer/src/YUV THEORAPLAYER_SRC_DIR3)
aux_source_directory(theoraplayer/src/YUV/C THEORAPLAYER_SRC_DIR4)
aux_source_directory(theoraplayer/src/YUV/libyuv THEORAPLAYER_SRC_DIR5)
aux_source_directory(theoraplayer/src/YUV/libyuv/src THEORAPLAYER_SRC_DIR6)
   
add_library (theoraplayer STATIC ${THEORAPLAYER_SRC_DIR1} ${THEORAPLAYER_SRC_DIR2} ${THEORAPLAYER_SRC_DIR3} ${THEORAPLAYER_SRC_DIR4} ${THEORAPLAYER_SRC_DIR5} ${THEORAPLAYER_SRC_DIR6})

# demos

list(APPEND DEMO_BASECODE
   demos/basecode/glut/glutBasecode.cpp
   demos/basecode/util/util.cpp)

# glut_player example

list(APPEND GLUT_PLAYER_SOURCES
   demos/glut_player/glut_player.cpp)
   
   
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
include_directories(${GLUT_INCLUDE_DIRS})
include_directories(${OpenGL_INCLUDE_DIRS})

add_executable (glut_player ${DEMO_BASECODE} ${GLUT_PLAYER_SOURCES})

add_definitions(-D_LINUX)

include_directories(
   "ogg/include"
   "vorbis/lib"
   "vorbis/include"
   "theora/include"
   "theoraplayer/include"
   "demos"
   "demos/basecode/util"
   )

target_link_libraries (glut_player theoraplayer)
target_link_libraries (glut_player theora)
target_link_libraries (glut_player vorbis)
target_link_libraries (glut_player ogg)
target_link_libraries (glut_player pthread)
target_link_libraries (glut_player ${OPENGL_LIBRARIES})
target_link_libraries (glut_player ${GLUT_LIBRARIES})
