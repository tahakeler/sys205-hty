CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -I./third_party

SRC_DIR = src
BIN_DIR = bin

all: csv_to_hty analyze

csv_to_hty: $(SRC_DIR)/csv_to_hty.cpp
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/csv_to_hty $(SRC_DIR)/csv_to_hty.cpp

analyze: $(SRC_DIR)/analyze.cpp
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/analyze $(SRC_DIR)/analyze.cpp
