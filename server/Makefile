# ===========================================
#   Key-Value Server (CS744 Project)
# ===========================================

# ---------- Compiler and Flags ----------
CXX       := g++
CXXFLAGS  := -std=c++17 -O2 -Wall -Iinclude -Iinclude/lib
LDFLAGS   := -lmysqlcppconn -lpthread

# ---------- Directories ----------
SRC_DIR   := src
OBJ_DIR   := obj
BIN       := kv_server

# ---------- Sources and Objects ----------
SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
OBJS      := $(SRCS:$(SRC_DIR)/%.cpp=$(OBJ_DIR)/%.o)

# ---------- Default Target ----------
all: $(BIN)

# ---------- Link Executable ----------
$(BIN): $(OBJS)
	@echo "🔗 Linking $@ ..."
	$(CXX) $(OBJS) -o $@ $(LDFLAGS)
	@echo "✅ Build complete: $@"

# ---------- Compile Each Source ----------
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "🧩 Compiling $< ..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------- Ensure obj Directory Exists ----------
$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# ---------- Clean ----------
clean:
	@echo "🧹 Cleaning build files..."
	rm -rf $(OBJ_DIR) $(BIN)

.PHONY: all clean
