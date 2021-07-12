TARGET = main
SRC = ./src
OBJ = ./obj

$(OBJ)/%.o: $(SRC)/%.c
	$(CXX) $(CFLAGS) -c $< -o $@ 
$(OBJ)/%.o: $(SRC)/%.cpp
	$(CXX) $(CXXFLAGS) -std=c++11 -c $< -o $@ 

SOURCES = $(wildcard $(SRC)/*.c $(SRC)/*.cpp)
OBJS = $(patsubst %c,$(OBJ)/%o,$(patsubst %.cpp,$(OBJ)/%.o,$(notdir $(SOURCES))))
$(TARGET) : $(OBJS)
	$(CXX) $(CFLAGS) -o $(TARGET) $^
.PHONY: clean
clean:
	-rm -f $(TARGET) $(OBJ)/*

