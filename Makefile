# -------- Compiler --------
CC = gcc
CFLAGS = -Wall -g

# -------- Directories --------
SRC = src
INCLUDE = include

# -------- Source Files --------
SERVER_SRC = $(SRC)/server.c \
             $(SRC)/dhcp_packet.c \
             $(SRC)/ip_lease.c \
             $(SRC)/logger.c

CLIENT_SRC = $(SRC)/client.c \
             $(SRC)/dhcp_packet.c \
             $(SRC)/logger.c

# -------- Output --------
SERVER_OUT = server
CLIENT_OUT = client

# -------- Build Rules --------
all: $(SERVER_OUT) $(CLIENT_OUT)

# Server build
$(SERVER_OUT): $(SERVER_SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE) -o $(SERVER_OUT) $(SERVER_SRC)

# Client build
$(CLIENT_OUT): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -I$(INCLUDE) -o $(CLIENT_OUT) $(CLIENT_SRC)

# -------- Clean --------
clean:
	rm -f $(SERVER_OUT) $(CLIENT_OUT)
	rm -f logs/*.log logs/*.log.1

# -------- Run --------
run-server:
	./$(SERVER_OUT)

run-client:
	./$(CLIENT_OUT) client1

# -------- Create Required Folders --------
setup:
	mkdir -p logs config

.PHONY: all clean run-server run-client setup
