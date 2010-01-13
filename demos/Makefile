

all:
	cd glut_player && make && cd ..
	cd sprite_animation && make && cd ..
	cd av_player && make && cd ..
	cd multiple_videos && make && cd ..
	@echo "-------------------"
	@echo "If compiling TV room does not work, install 'libdevil' and add -l flag to LDFLAGS"
	@echo "Proceeding in 2 seconds"
	@echo "-------------------"
	@sleep 2
	-cd tv_room && make && cd ..
	@echo "-------------------"
	@echo "If compiling TV room did not work, install 'libdevil' and add -l flag to LDFLAGS"
	@echo "Proceeding in 2 seconds"
	@echo "-------------------"
	@sleep 2
clean:
	cd glut_player && make clean && cd ..
	cd sprite_animation && make clean && cd ..
	cd av_player && make clean && cd ..
	cd multiple_videos && make clean && cd ..
	cd tv_room && make clean && cd ..
