# Builds the usecode library, tools and tests with a plain C compiler.
# CMakeLists.txt builds the same things for those who prefer CMake.

CC ?= cc
AR ?= ar
CFLAGS ?= -O2
PREFIX ?= /usr/local
BUILD ?= build

# Where `make usecode` and `make check` write; PROJECT names the project.u7p to build.
OUT ?= $(BUILD)/usecode

WARNINGS = -Wall -Wextra -Wpedantic -Werror -Wconversion -Wshadow \
	-Wstrict-prototypes -Wmissing-prototypes
ALL_CFLAGS = -std=c17 $(WARNINGS) -Iinclude $(CFLAGS)

LIB = $(BUILD)/libu7usecode.a
LIB_OBJECTS = $(patsubst src/%.c,$(BUILD)/obj/%.o,$(wildcard src/*.c))

TOOLS = u7ucproj u7uccomp u7uclink u7ucdec u7ucdis u7ucverify u7ucround u7ucflow u7ucsym
TESTS = test_normalize test_stack_flow test_expression test_emit test_lower \
	test_parse test_generate test_sha256 test_compile

.PHONY: all test usecode check install clean

all: $(TOOLS:%=$(BUILD)/%)

$(BUILD)/obj/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(ALL_CFLAGS) -MMD -MP -c $< -o $@

$(LIB): $(LIB_OBJECTS)
	$(AR) rcs $@ $^

$(BUILD)/%: tools/%.c $(LIB)
	$(CC) $(ALL_CFLAGS) $< $(LIB) -o $@

$(BUILD)/%: tests/%.c $(LIB)
	$(CC) $(ALL_CFLAGS) $< $(LIB) -o $@

test: all $(TESTS:%=$(BUILD)/%)
	@for t in $(TESTS); do $(BUILD)/$$t || exit 1; done
	sh tests/project_edit.sh $(BUILD)/u7ucproj $(BUILD)/u7ucdec $(BUILD)/project_edit

usecode: $(BUILD)/u7ucproj
	@test -n "$(PROJECT)" || { echo "set PROJECT=path/to/project.u7p"; exit 1; }
	@mkdir -p $(OUT)
	$(BUILD)/u7ucproj $(PROJECT) $(OUT)

# Also fails unless USECODE matches the project's expected sha256.
check: $(BUILD)/u7ucproj
	@test -n "$(PROJECT)" || { echo "set PROJECT=path/to/project.u7p"; exit 1; }
	@mkdir -p $(OUT)
	$(BUILD)/u7ucproj --check $(PROJECT) $(OUT)

install: all
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(TOOLS:%=$(BUILD)/%) $(DESTDIR)$(PREFIX)/bin/

clean:
	rm -rf $(BUILD)

-include $(LIB_OBJECTS:.o=.d)
