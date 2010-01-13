OBJS=tv_room.o ../OpenAL_AudioInterface.o ../ObjModel.o

all: tv_room

tv_room: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o tv_room

clean:
	rm -f tv_room $(OBJS)
