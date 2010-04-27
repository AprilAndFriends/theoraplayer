OBJS=av_player.o ../OpenAL_AudioInterface.o

all: av_player

av_player: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o av_player

clean:
	rm -f av_player $(OBJS)
