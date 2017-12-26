
#CC = arm-linux-gnueabihf-gcc
#CPP = arm-linux-gnueabihf-g++
CPP = g++

all:server tcp_client

server:server.o
	$(CPP) server.o -o server
server.o:server.cpp
	$(CPP) -c server.cpp -o server.o

tcp_client:tcp_client.o
	$(CPP) tcp_client.o -o tcp_client
tcp_client.o:tcp_client.cpp bufhead.h CRC16.h
	$(CPP) -c tcp_client.cpp -o tcp_client.o

#tcp_server:tcp_server.o uart_test.o key.o
#	$(CC) tcp_server.o uart_test.o key.o -o tcp_server -lpthread
#tcp_server.o:tcp_server.c uart_test.h key.h CRC16.h
#	$(CC) -c tcp_server.c -o tcp_server.o -lpthread
#uart_test.o:uart_test.c uart_test.h bufhead.h log.h
#	$(CC) -c uart_test.c -o uart_test.o
#key.o:key.c key.h
#	$(CC) -c key.c -o key.o

clean:
	rm -f tcp_client  server  mpu6050 *.o
	
