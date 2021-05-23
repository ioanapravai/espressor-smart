espressor: main.cpp
	g++ $< -o $@ -lpistache -lcrypto -lssl -lpthread
