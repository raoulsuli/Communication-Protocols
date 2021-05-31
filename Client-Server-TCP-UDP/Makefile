# Makefile

CFLAGS = -Wall -g

# Portul pe care asculta serverul (de completat)
PORT = 1500

# Adresa IP a serverului
IP_SERVER = 127.0.0.1

# ID - ul subscriber-ului (de completat)
ID_SUBSCRIBER = 2000

all: server subscriber

# Compileaza server.cpp
server: server.cpp

# Compileaza subscriber.cpp
subscriber: subscriber.cpp

.PHONY: clean run_server run_subscriber

# Ruleaza serverul
run_server:
	./server ${PORT}

# Ruleaza clientul
run_subscriber:
	./subscriber ${IP_SERVER} ${PORT}

# Sterge executabile
clean:
	rm -f server subscriber
