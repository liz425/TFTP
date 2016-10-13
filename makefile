all: Server
Server: main.cpp Header.h
	g++ -std=c++11 main.cpp -lpthread -o Server
.PHONY: clean
clean:
	-rm -f Client Server
