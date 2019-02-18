proxy: ./src/main.cpp ./src/*.h
	g++ -ggdb3 -std=gnu++11 -pthread -pedantic -Wall -Werror -o proxy \
	./src/main.cpp -I /usr/local/boost/include -I /usr/local/include \
	-L /usr/local/boost/lib -L /usr/local/lib\
	-lboost_system -lboost_regex -lglog 
clean:
	rm -rf proxy *~
