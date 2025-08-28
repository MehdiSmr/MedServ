all: clean classes

clean:
	rm -f client server user/user

classes:
	clang++ -Wall -o client client.cpp
	clang++ -Wall -o server server.cpp

