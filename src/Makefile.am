AM_CXXFLAGS=-I../include/

lib_LTLIBRARIES = libtheoraplayer.la
libtheoraplayer_la_SOURCES = \
	TheoraAudioInterface.cpp	TheoraTimer.cpp			TheoraVideoManager.cpp \
	TheoraFrameQueue.cpp		TheoraVideoClip.cpp		TheoraWorkerThread.cpp \
	TheoraVideoFrame.cpp		TheoraAsync.cpp			TheoraDataSource.cpp \
	TheoraException.cpp			TheoraUtil.cpp
libtheoraplayer_la_LDFLAGS=-module -avoid-version
if DARWIN
libtheoraplayer_la_LDFLAGS += -shrext .bundle
endif DARWIN


