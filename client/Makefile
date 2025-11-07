CXX = g++
CXXFLAGS = -std=c++17 -Wall -I../server/include -Iinclude
LDFLAGS = -lpthread
SRC = kv_client.cpp
BIN = kv_client

all: $(BIN)

$(BIN): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(BIN) $(LDFLAGS)

clean:
	rm -f $(BIN)

.PHONY: all clean
