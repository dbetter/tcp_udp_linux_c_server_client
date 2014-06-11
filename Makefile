CCC = gcc
CCFLAGS = -g -Wall
CCLINK = $(CCC)
OBJS1 = client.o
OBJS2 = server.o


all: client server
# Creating the client executable
client: $(OBJS1)
	$(CCLINK) -o client $(OBJS1)

# Creating the server executable
server: $(OBJS2)
	$(CCLINK) -o server $(OBJS2)

# Creating object files using default rules

client.o: client.c
server.o: server.c

# Cleaning old files before new make
clean:
	rm -f *.o
	rm -f client
	rm -f server
	
