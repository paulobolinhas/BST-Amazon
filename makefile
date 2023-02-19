BINARY = binary
INCLUDE = include
LIB = lib
OBJECT = object
SOURCE = source
CC = gcc
CFLAGS = -Wall -lpthread -g -o 
CLIENT_LIB = data.o entry.o client_stub.o network_client.o message.o
TREE_CLIENT = tree_client.o sdmessage.pb-c.o
TREE_SERVER = tree_server.o network_server.o tree_skel.o client_stub.o network_client.o message.o sdmessage.pb-c.o data.o entry.o
LDFLAGS = /usr/lib/x86_64-linux-gnu/libprotobuf-c.a -DTHREADED -lzookeeper_mt
PROTOC= protoc --c_out=.

all: proto client-lib.o tree_client tree_server protoclean

%.o: $(SOURCE)/%.c $($@)
	$(CC) -Wall -g -I include -o $(OBJECT)/$@ -c $<

proto: 
	$(PROTOC) sdmessage.proto
	$(CC) -c sdmessage.pb-c.c
	cp sdmessage.pb-c.o $(OBJECT)/sdmessage.pb-c.o
	cp sdmessage.pb-c.c $(SOURCE)/sdmessage.pb-c.c
	cp sdmessage.pb-c.h $(INCLUDE)/sdmessage.pb-c.h

client-lib.o: $(CLIENT_LIB)
	ld -r $(addprefix $(OBJECT)/,$(CLIENT_LIB)) -o $(LIB)/client-lib.o


tree_client: $(TREE_CLIENT)
	$(CC) $(CFLAGS) $(BINARY)/tree_client $(addprefix $(OBJECT)/,$(TREE_CLIENT)) $(LIB)/client-lib.o $(LDFLAGS)


tree_server: $(TREE_SERVER)
	$(CC) $(CFLAGS) $(BINARY)/tree_server $(OBJECT)/tree.o $(addprefix $(OBJECT)/,$(TREE_SERVER)) $(LDFLAGS) 

protoclean:
	rm sdmessage.pb-c.o 
	rm sdmessage.pb-c.c 
	rm sdmessage.pb-c.h 


clean:
	rm -f $(OBJECT)/*
	rm -f $(BINARY)/*
	rm -f $(LIB)/* 