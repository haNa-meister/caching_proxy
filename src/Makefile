proxy: main.cpp *.h
	g++ -ggdb3 -std=gnu++11 -pthread -pedantic -Wall -Werror -o proxy \
	main.cpp -lboost_system -lboost_regex -lglog 
clean:
	rm -rf proxy *~
