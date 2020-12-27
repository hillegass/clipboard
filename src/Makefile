CFLAGS = -ggdb
CXXFLAGS = -ggdb
EXE = clipd
OBJS = clipd.o clip_common.o store.o

$(EXE): $(OBJS)
	gcc $^ -lstdc++ -lsystemd -o $@

clean:
	rm -rf $(EXE) $(OBJS)
