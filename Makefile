#TARGET = main
#SRC = ./src
#OBJ = ./obj

#$(OBJ)/%.o: $(SRC)/%.c
#	$(CXX) $(CFLAGS) -c $< -o $@ 
#$(OBJ)/%.o: $(SRC)/%.cpp
#	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@ 

#SOURCES = $(wildcard $(SRC)/*.c $(SRC)/*.cpp)
#OBJS = $(patsubst %c,$(OBJ)/%o,$(patsubst %.cpp,$(OBJ)/%.o,$(notdir $(SOURCES))))
#$(TARGET) : $(OBJS)
#	$(CXX) $(CFLAGS) -o $(TARGET) $^
#.PHONY: clean
#clean:
#	-rm -f $(TARGET) $(OBJ)/*

#CXX      := -c++
#CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -std=c++11
CFLAGS 	 := -std=c99
CXXFLAGS := -std=c++11
#LDFLAGS  := -L/usr/lib -lstdc++ -lm
LDFLAGS  := -luci
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
TARGET   := main
INCLUDE  := -Iinclude/
SRC_DIRS := $(shell find ./src -maxdepth 5 -type d)
SRC_C    := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
SRC_CXX  := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
SRC		 := $(SRC_C) $(SRC_CXX)

OBJECTS  := $(SRC_C:%.c=$(OBJ_DIR)/%.o) $(SRC_CXX:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES \
	:= $(OBJECTS:.o=.d)

all: build $(APP_DIR)/$(TARGET)

$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(OBJ_DIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) $(INCLUDE) -c $< -MMD -o $@

$(APP_DIR)/$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -o $(APP_DIR)/$(TARGET) $^ $(LDFLAGS)

-include $(DEPENDENCIES)

.PHONY: all build clean debug release info

build:
	@clear
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

debug: CXXFLAGS += -DDEBUG -g -O0
debug: CFLAGS += -DDEBUG -g -O0
debug: all

release: CXXFLAGS += -O2
release: CFLAGS += -O2
release: all

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"
