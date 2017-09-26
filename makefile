all: client server

client: client.cpp
	g++ -o client client.cpp -pthread
server: server.cpp
	g++ -o server server.cpp -pthread

clean:
	rm server client
