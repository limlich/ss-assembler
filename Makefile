TARGET := assembler

BUILD_DIR := build
SRC_DIR := src
INC_DIR := inc

PARSER_Y := parser.y
PARSER_H := $(INC_DIR)/parser.hpp
PARSER_LOC_H := $(INC_DIR)/location.hpp
PARSER_SRC := $(SRC_DIR)/parser.cpp
PARSER_OBJ := $(PARSER_SRC:%=$(BUILD_DIR)/%.o)

LEXER_L := lexer.l
LEXER_SRC := $(SRC_DIR)/lexer.cpp
LEXER_OBJ := $(LEXER_SRC:%=$(BUILD_DIR)/%.o)

SRCS = $(shell find $(SRC_DIR) -name *.cpp)
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
LIBS := 

INC_FLAGS := $(addprefix -I,$(INC_DIR))
CXX_FLAGS := -g -std=c++17 -Wall -Wextra $(INC_FLAGS) -MMD -MP
LD_FLAGS := -g $(addprefix -l,$(LIBS))

$(BUILD_DIR)/$(TARGET): $(PARSER_OBJ) $(LEXER_OBJ) $(OBJS)
	$(CXX) $(LD_FLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c $< -o $@

# bison rule
$(PARSER_H) $(PARSER_LOC_H) $(PARSER_SRC): $(PARSER_Y)
	mkdir -p $(INC_DIR) $(SRC_DIR)
	bison -o $(PARSER_SRC) --defines=$(PARSER_H) $<
	mv $(SRC_DIR)/location.hpp $(PARSER_LOC_H)

# flex rule
$(LEXER_SRC): $(LEXER_L)
	mkdir -p $(dir $@)
	flex -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
