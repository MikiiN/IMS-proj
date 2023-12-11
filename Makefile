CXX = g++
CXX_FLAGS = -I/usr/local/include -L/usr/local/lib64 -lsimlib -lm -Wall

modelStation: main.cpp
	$(CXX) $(CXX_FLAGS) main.cpp -o modelStation

run: modelStation
	./modelStation
	./modelStation exp2
	./modelStation exp3

clean:
	rm -rf modelStation modelStation*.out

setPath:
	export LD_LIBRARY_PATH=/usr/local/lib/