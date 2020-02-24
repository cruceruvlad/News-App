all: server subscriber

server: 
	g++ -std=c++17 -Werror server.cpp -o server

subscriber: 
	g++ -std=c++17 -Werror subscriber.cpp -o subscriber

clean:	
	rm -f server subscriber
