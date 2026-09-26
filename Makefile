

LIB=dlysym-intercept.o




ARCH ?=$(shell $(CC) $(CFLAGS) -dumpmachine|cut -d - -f1)

SRC_DIR=src
BUILD_DIR=build
LIB_SRCS=$(SRC_DIR)/init_loader.c $(SRC_DIR)/hook.cpp $(SRC_DIR)/jumptable.c \
	$(SRC_DIR)/search_symbols.c \
	$(SRC_DIR)/arch/$(ARCH)/jumptable_entry.s $(SRC_DIR)/arch/$(ARCH)/get_elf_header.s


EXAMPLE_LIB=example-lib.so
EXAMPLE_PROG=example-prog
EXAMPLE_SRC_DIR=example
EXAMPLE_BUILD_DIR=$(BUILD_DIR)/$(EXAMPLE_SRC_DIR)
EXAMPLE_LIB_SRCS=$(EXAMPLE_SRC_DIR)/example_lib.c
EXAMPLE_PROG_SRCS=$(EXAMPLE_SRC_DIR)/example_prog.c


DEBUG ?= 0
MAXERRORS ?= 1
INCLUDE_DIR=-I./include


CFLAGS =-Wall -Wextra -Werror -std=gnu11 -fpic $(INCLUDE_DIR)
CXXFLAGS =-Wall -Wextra -Werror -std=gnu++11 -fpic $(INCLUDE_DIR)
LDFLAGS=
LDLIBS=-lelf


BUILD_SUBDIRS=$(BUILD_DIR)/arch/$(ARCH)


LIB_OBJS = $(patsubst src/%, $(BUILD_DIR)/%, $(addsuffix .o,$(basename $(LIB_SRCS))))
EXAMPLE_LIB_OBJS := $(patsubst $(EXAMPLE_SRC_DIR)/%, $(EXAMPLE_BUILD_DIR)/%, $(addsuffix .o,$(basename $(EXAMPLE_LIB_SRCS))))
EXAMPLE_PROG_OBJS := $(patsubst $(EXAMPLE_SRC_DIR)/%, $(EXAMPLE_BUILD_DIR)/%, $(addsuffix .o,$(basename $(EXAMPLE_PROG_SRCS))))


# detect used compilers
CC_COMPILER=$(notdir $(shell readlink -f $(shell which $(CC))))
CXX_COMPILER=$(notdir $(shell readlink -f $(shell which $(CXX))))


ifneq (,$(findstring gcc, $(CC_COMPILER)))
CFLAGS+=-fmax-errors=$(MAXERRORS)
else ifneq (,$(findstring clang, $(CC_COMPILER)))
CFLAGS+=-ferror-limit=$(MAXERRORS)
endif


ifneq (,$(findstring gnu-g++, $(CXX_COMPILER)))
CXXFLAGS+=-fmax-errors=$(MAXERRORS)
else ifneq (,$(findstring clang, $(CXX_COMPILER)))
CXXFLAGS+=-ferror-limit=$(MAXERRORS)
endif


ifeq ($(DEBUG), 0)
	CFLAGS+=-O3 -fvisibility=hidden
	CXXFLAGS+=-O3 -fvisibility=hidden
	LDFLAGS+=-s
else
	CFLAGS+=-DDEBUG -g -O0
	CXXFLAGS+=-DDEBUG -g -O0
endif


.PHONY: all clean example

all:  $(LIB)

example: $(EXAMPLE_LIB) $(EXAMPLE_PROG)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_SUBDIRS)/
	$(CXX) $(CXXFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_SUBDIRS)/
	$(CC) $(CFLAGS) -o $@ -c $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.s | $(BUILD_SUBDIRS)/
	$(CC) $(CFLAGS) -o $@ -c $<

$(EXAMPLE_BUILD_DIR)/%.o: $(EXAMPLE_SRC_DIR)/%.c | $(EXAMPLE_BUILD_DIR)/
	$(CC) $(CFLAGS) -o $@ -c $<

$(LIB): $(LIB_OBJS)
	$(LD) -r -o $@ $^

$(EXAMPLE_LIB): $(LIB) $(EXAMPLE_LIB_OBJS)
	$(CXX) -shared $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(EXAMPLE_PROG): $(EXAMPLE_PROG_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

%/:
	mkdir -p $@

clean:
	rm -fr $(BUILD_DIR) $(LIB) $(EXAMPLE_LIB) $(EXAMPLE_PROG) $(EXAMPLE_LIB_OBJS) $(EXAMPLE_PROG_OBJS)
