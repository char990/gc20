TARGET   := goblin
LDFLAGS  := -luci -lcrypto
#LDFLAGS  := -L/usr/lib -lstdc++ -lm

#CXX      := -c++
#CXXFLAGS := -pedantic-errors -Wall -Wextra -Werror -std=c++11
BUILDTIME := $(shell date +"%b %d %Y %T %Z")
CFLAGS 	 := -std=gnu11 -D '__BUILDTIME__="$(BUILDTIME)"'
CXXFLAGS := -std=c++11 -D '__BUILDTIME__="$(BUILDTIME)"'
BUILD    := ./build
OBJ_DIR  := $(BUILD)/objects
APP_DIR  := $(BUILD)/apps
INCLUDE  := -Iinclude/
SRC_ROOT := ./src
SRC_DIRS := $(shell find $(SRC_ROOT) -maxdepth 5 -type d)
SRC_C    := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.c))
SRC_CXX  := $(foreach dir, $(SRC_DIRS), $(wildcard $(dir)/*.cpp))
SRC		 := $(SRC_C) $(SRC_CXX)

OBJECTS  := $(SRC_C:%.c=$(OBJ_DIR)/%.o) $(SRC_CXX:%.cpp=$(OBJ_DIR)/%.o)
DEPENDENCIES := $(OBJECTS:.o=.d)

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

.PHONY: all build clean debug release info pack

build:
	@touch $(SRC_ROOT)/main.cpp
	@mkdir -p $(APP_DIR)
	@mkdir -p $(OBJ_DIR)

pack:
	-@./pack

debug: 
	@echo "debug : $(BUILDTIME)" > buildtime
debug: CXXFLAGS += -DDEBUG -g -O0
debug: CFLAGS += -DDEBUG -g -O0
debug: all
debug: pack

release: 
	@echo "release : $(BUILDTIME)" > buildtime
release: CXXFLAGS += -O2 -DNDEBUG
release: CFLAGS += -O2 -DNDEBUG
release: all
release: pack

clean:
	-@rm -rvf $(OBJ_DIR)/*
	-@rm -rvf $(APP_DIR)/*

info:
	@echo "[*] Application dir: ${APP_DIR}     "
	@echo "[*] Object dir:      ${OBJ_DIR}     "
	@echo "[*] Sources:         ${SRC}         "
	@echo "[*] Objects:         ${OBJECTS}     "
	@echo "[*] Dependencies:    ${DEPENDENCIES}"

