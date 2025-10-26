CXX = g++
CXXFLAGS = -std=c++17 -g -Wall -I. -Isrc/app/encryptDecrypt -Isrc/app/FileHandling -Isrc/app/processes -Isrc/app/threads

MAIN_TARGET = encrypt_decrypt
CRYPTION_TARGET = cryption
THREAD_TARGET = encrypt_decrypt_mt

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

MAIN_OBJ = $(MAIN_SRC:.cpp=.o)
CRYPTION_OBJ = $(CRYPTION_SRC:.cpp=.o)
# For threads, compile Cryption.cpp separately with -DMULTITHREAD
THREAD_OBJ = main_mt.o \
             src/app/threads/ThreadManagement.o \
             src/app/FileHandling/IO.o \
             src/app/FileHandling/ReadEnv.o \
             Cryption_mt.o \
             BenchmarkLogger2.o

all: $(MAIN_TARGET) $(CRYPTION_TARGET) $(THREAD_TARGET)

$(MAIN_TARGET): $(MAIN_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(CRYPTION_TARGET): $(CRYPTION_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(THREAD_TARGET): $(THREAD_OBJ)
	$(CXX) $(CXXFLAGS) $^ -o $@ -lpthread

Cryption_mt.o: src/app/encryptDecrypt/Cryption.cpp
	$(CXX) $(CXXFLAGS) -DMULTITHREAD -c $< -o $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(MAIN_OBJ) $(CRYPTION_OBJ) $(THREAD_OBJ) $(MAIN_TARGET) $(CRYPTION_TARGET) $(THREAD_TARGET) Cryption_mt.o
	@echo "🧹 Cleaned all build artifacts."

.PHONY: clean all