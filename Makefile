# ========================================
# Target name

TARGET = ntp_cli_test

#========================================
# Compiler

CC = g++

#========================================
# Options

CXXFLAGS = -Wall -O2 -std=c++98
CXXMACRO = -DNTP_OUTPUT=1
#LDFLAGS = -pthread

#========================================
# Modules

CORE_SRC = $(wildcard *.cpp)
CORE_OBJ = $(CORE_SRC:%.cpp=%.o)

OBJS = $(CORE_OBJ)

#========================================
# Build

$(TARGET):$(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) -o $(TARGET)

$(CORE_OBJ):%.o:%.cpp
	$(CC) $(CXXFLAGS) $(CXXMACRO) -c $^ -o $@

#========================================
# Cleanup

clean:
	@rm -vf $(OBJS) $(TARGET)

#========================================

.PHONY:clean
