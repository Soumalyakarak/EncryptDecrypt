CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -I. -Isrc/app/encryptDecrypt -Isrc/app/FileHandling -Isrc/app/processes -Isrc/app/threads

# OpenSSL libraries
LIBS = -lssl -lcrypto

# Build directory
BUILD_DIR = build

MAIN_TARGET = $(BUILD_DIR)/encrypt_decrypt
CRYPTION_TARGET = $(BUILD_DIR)/cryption
THREAD_TARGET = $(BUILD_DIR)/encrypt_decrypt_mt

MAIN_SRC = main.cpp \
           src/app/processes/ProcessManagement.cpp \
           src/app/FileHandling/IO.cpp \
           src/app/FileHandling/ReadEnv.cpp \
           src/app/encryptDecrypt/Cryption.cpp \
           BenchmarkLogger.cpp  

CRYPTION_SRC = src/app/encryptDecrypt/CryptionMain.cpp \
               src/app/encryptDecrypt/Cryption.cpp \
               src/app/FileHandling/IO.cpp \
               src/app/FileHandling/ReadEnv.cpp \
               BenchmarkLogger.cpp

THREAD_SRC = main_mt.cpp \
             src/app/threads/ThreadManagement.cpp \
             src/app/FileHandling/IO.cpp \
             src/app/FileHandling/ReadEnv.cpp \
             BenchmarkLogger2.cpp

# Object files inside build/
MAIN_OBJ = $(addprefix $(BUILD_DIR)/, $(MAIN_SRC:.cpp=.o))
CRYPTION_OBJ = $(addprefix $(BUILD_DIR)/, $(CRYPTION_SRC:.cpp=.o))

THREAD_OBJ = $(BUILD_DIR)/main_mt.o \
             $(BUILD_DIR)/src/app/threads/ThreadManagement.o \
             $(BUILD_DIR)/src/app/FileHandling/IO.o \
             $(BUILD_DIR)/src/app/FileHandling/ReadEnv.o \
             $(BUILD_DIR)/Cryption_mt.o \
             $(BUILD_DIR)/BenchmarkLogger2.o

all: $(MAIN_TARGET) $(CRYPTION_TARGET) $(THREAD_TARGET)

# Link targets
$(MAIN_TARGET): $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(CRYPTION_TARGET): $(CRYPTION_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ $(LIBS)

$(THREAD_TARGET): $(THREAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread $(LIBS)

# Special multithread Cryption
$(BUILD_DIR)/Cryption_mt.o: src/app/encryptDecrypt/Cryption.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -DMULTITHREAD -c $< -o $@

# Generic rule for all .o files
$(BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean
clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		rm -rf $(BUILD_DIR)/* $(BUILD_DIR)/.* 2>/dev/null || true; \
	fi
	@echo "Cleaned build contents"

.PHONY: clean all