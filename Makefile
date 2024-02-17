BUILD?=debug

BINS=$(patsubst src/bin/%.c,target/$(BUILD)/%,$(wildcard src/bin/*.c))
SOURCES=$(shell find src/ -not \( -path 'src/bin' -prune \) -type f -name '*.c')
OBJS=$(patsubst src/%.c,target/$(BUILD)/.objs/%.o,$(SOURCES))

CFLAGS += -Iinclude

ifeq ($(BUILD),debug)
CFLAGS += -g
LDFLAGS += -g
else ifeq ($(BUILD),release)
CFLAGS += -O2 -pipe -s
LDFLAGS += -s
else
$(error build type not specified)
endif

.PHONY: all shared static clean tests vars
.SUFFIXES: .c .o

all: shared static $(BINS)

shared: target/$(BUILD)/libcrequest.so

static: target/$(BUILD)/libcrequest.a

target/$(BUILD)/.objs/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	@echo CC  $@
	@$(CC) $(CFLAGS) -c -o $@ $<

target/$(BUILD)/%: src/bin/%.c target/$(BUILD)/libcrequest.a
	@mkdir -p $(shell dirname $@)
	@echo LD  $@
	@$(CC) $(CFLAGS) -o $@ $^

target/$(BUILD)/libcrequest.so: $(OBJS)
	@echo LD  $@
	@$(LD) $(LDFLAGS) -shared -o $@ $^

target/$(BUILD)/libcrequest.a: $(OBJS)
	@echo AR  $@
	@$(AR) $(ARFLAGS) $@ $(OBJS) >/dev/null 2>&1

tests: static
	@CFLAGS="$(CFLAGS)" BUILD="$(BUILD)" ./scripts/run_tests.sh $(SOURCES)

vars:
	@printf 'BUILD=%s\n' "$(BUILD)"
	@printf 'BINS=%s\n' "$(BINS)"
	@printf 'SOURCES=%s\n' "$(SOURCES)"
	@printf 'OBJS=%s\n' "$(OBJS)"
	@printf 'CFLAGS=%s\n' "$(CFLAGS)"

compile_commands.json:
	./scripts/compile_commands.sh

clean:
	@rm -rf target/

