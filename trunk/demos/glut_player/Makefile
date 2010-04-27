OBJS=glut_player.o

all: glut_player

glut_player: $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) $(LDFLAGS) -o glut_player

clean:
	rm -f glut_player $(OBJS)
