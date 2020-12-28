CFLAGS = -I../src -ggdb
CXXFLAGS = -I../src -ggdb

all: provider_test store_test reader_test watcher_test

provider_test: clipboard.o clip_common.o provider_test.o
	gcc $^ -lsystemd -o $@

reader_test: clipboard.o clip_common.o reader_test.o
	gcc $^ -lsystemd -o $@

watcher_test: clipboard.o clip_common.o watcher_test.o
	gcc $^ -lsystemd -o $@

store_test: store.o clip_common.o store_test.o
	gcc $^ -lstdc++ -o $@

%.o: ../src/%.c
	gcc -c -ggdb -I.. -o $@ $<

%.o: ../src/%.cpp
	g++ -c -ggdb -I.. -o $@ $<

clean:
	rm -rf *.o store_test provider_test reader_test watcher_test
