proxy: ./src/main.cpp ./src/*.h
	g++ -ggdb3 -std=gnu++11 -pthread -pedantic -Wall -Werror -o proxy \
	./src/main.cpp -I /usr/local/opt/boost/include -L /usr/local/opt/boost/lib \
	-lboost_system -lboost_date_time -lboost_regex
clean:
	rm -rf proxy *~
