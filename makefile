all: clean classes

clean:
	rm -f client server user/user

classes:
	clang++ -Werror -std=c++20 -o client client.cpp
	clang++ -Werror -std=c++20 -o server server.cpp message/message.cpp chat/chat.cpp user/user.cpp

