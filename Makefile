# -------- Compiler --------
CC = gcc
CFLAGS = -Wall 

SERVER_OUT = server
CLIENT_OUT = client

# -------- Build Rules --------
all: server client

server: 
	$(CC) $(CFLAGS) src/server.c src/dhcp_packet.c src/ip_lease.c src/logger.c -o server
client:
	$(CC) $(CFLAGS) src/client.c src/dhcp_packet.c src/logger.c src/ip_lease.c -o client
clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)




