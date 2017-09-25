all: client server

client: client.cc packet.cc
	g++ -c packet.cc
	g++ -c client.cc
	g++ -o client packet.o client.o

server: server.cc packet.cc
	g++ -c packet.cc
	g++ -c server.cc
	g++ -pthread -o server packet.o server.o

clean: 
	rm server client *.o
