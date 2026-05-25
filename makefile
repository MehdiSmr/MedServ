all: clean classes

clean:
	rm -f client servert user/user

classes:
	clang++ -Werror -std=c++20 -o client utils/utils.cpp client.cpp
	clang++ -Werror -std=c++20 -o servert servert.cpp threadp/threadp.cpp utils/utils.cpp message/message.cpp chat/chat.cpp user/user.cpp
	clang++ -Werror -std=c++20 -o serverp serverp.cpp threadp/threadp.cpp utils/utils.cpp message/message.cpp chat/chat.cpp user/user.cpp
