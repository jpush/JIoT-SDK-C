#CC=gcc
CFLAGS = -Wall -g
        
INCDIR +=  -I../../include/jclient/  
LIBDIR += 

TARGET = demo

all:
	$(CC) $(CFLAGS) $(INCDIR) -o $(TARGET) demo.c  ../../lib/libjiot_c_sdk.a  $(LIBDIR) -lpthread  -lssl -lcrypto
    
clean:
	rm $(TARGET)
