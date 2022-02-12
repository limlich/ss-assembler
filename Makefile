TARGET := assembler
DEBUG_ENABLED = 1

BUILD_DIR := build
SRC_DIR := src
INC_DIR := inc

PARSER_Y := $(SRC_DIR)/parser.y
PARSER_H := $(INC_DIR)/parser.hpp
PARSER_LOC_H := $(INC_DIR)/location.hpp
PARSER_SRC := $(SRC_DIR)/parser.cpp
PARSER_OBJ := $(PARSER_SRC:%=$(BUILD_DIR)/%.o)

LEXER_L := $(SRC_DIR)/lexer.l
LEXER_SRC := $(SRC_DIR)/lexer.cpp
LEXER_OBJ := $(LEXER_SRC:%=$(BUILD_DIR)/%.o)

SRCS = $(shell find $(SRC_DIR) -name *.cpp)
OBJS = $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS = $(OBJS:.o=.d)
LIBS := 

DEBUG_FLAGS = -g

INC_FLAGS := $(addprefix -I,$(INC_DIR))
CXX_FLAGS := -std=c++17 -Wall -Wextra $(INC_FLAGS) -MMD -MP
LD_FLAGS := $(addprefix -l,$(LIBS))
ifeq ($(DEBUG_ENABLED),1)
CXX_FLAGS += $(DEBUG_FLAGS)
LD_FLAGS += $(DEBUG_FLAGS)
endif

$(BUILD_DIR)/$(TARGET): $(PARSER_OBJ) $(LEXER_OBJ) $(OBJS)
	$(CXX) $(LD_FLAGS) $(OBJS) -o $@

$(BUILD_DIR)/%.cpp.o: %.cpp Makefile
	mkdir -p $(dir $@)
	$(CXX) $(CXX_FLAGS) -c $< -o $@

# bison rule
$(PARSER_H) $(PARSER_LOC_H) $(PARSER_SRC): $(PARSER_Y) Makefile
	mkdir -p $(INC_DIR) $(SRC_DIR)
	bison -o $(PARSER_SRC) --defines=$(PARSER_H) $<
	mv $(SRC_DIR)/location.hpp $(PARSER_LOC_H)

# flex rule
$(LEXER_SRC): $(LEXER_L) Makefile
	mkdir -p $(dir $@)
	flex -o $@ $<

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR) $(LEXER_SRC) $(PARSER_H) $(PARSER_LOC_H) $(PARSER_SRC)

-include $(DEPS)
