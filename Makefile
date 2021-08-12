TARGET := assembler

BUILD_DIR := build
SRC_DIR := src
INC_DIR := inc

SRCS := $(shell find $(SRC_DIR) -name *.cpp)
OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
LIBS := 

INC_FLAGS := $(addprefix -I,$(INC_DIR))
CXX_FLAGS := -g -std=c++17 -Wall -Wextra $(INC_FLAGS) -MMD -MP
LD_FLAGS := -g $(addprefix -l,$(LIBS))

$(BUILD_DIR)/$(TARGET): $(OBJS)
	$(CXX) $(LD_FLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c $< -o $@

# bison rule
$(INC_DIR)/parser.hpp $(INC_DIR)/location.hpp $(SRC_DIR)/parser.cpp: parser.y
	mkdir -p $(INC_DIR) $(SRC_DIR)
	bison -o $(SRC_DIR)/parser.cpp --defines=$(INC_DIR)/parser.hpp parser.y
	mv $(SRC_DIR)/location.hpp $(INC_DIR)

# flex rule
$(SRC_DIR)/lexer.cpp: lexer.l
	mkdir -p $(dir $@)
	flex -o $@ lexer.l

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
