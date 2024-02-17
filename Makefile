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
$(error build type specified is not defined (change the BUILD variable))
endif

.PHONY: all depend release debug shared static clean tests vars
.SUFFIXES: .c .o

all: depend shared static $(BINS)

clean:
	@rm -rf target/

release:
	@BUILD=release $(MAKE) --no-print-directory

debug:
	@BUILD=debug $(MAKE) --no-print-directory

shared: target/$(BUILD)/librequests.so

static: target/$(BUILD)/librequests.a

depend: .depend

.depend: $(SOURCES)
	@mkdir -p $(shell dirname $@)
	@rm -f "$@"
	@echo "CPP   .depend"
	@$(CC) $(CFLAGS) -MM $^ > "$@"

include .depend

target/$(BUILD)/.objs/%.o: src/%.c
	@mkdir -p $(shell dirname $@)
	@echo "CC    $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

target/$(BUILD)/%: src/bin/%.c target/$(BUILD)/librequests.a
	@mkdir -p $(shell dirname $@)
	@echo "LD    $@"
	@$(CC) $(CFLAGS) -o $@ $^

target/$(BUILD)/librequests.so: $(OBJS)
	@echo "LD    $@"
	@$(LD) $(LDFLAGS) -shared -o $@ $^

target/$(BUILD)/librequests.a: $(OBJS)
	@echo "AR    $@"
	@$(AR) $(ARFLAGS) $@ $(OBJS) >/dev/null 2>&1

tests: static
	@CFLAGS="$(CFLAGS)" BUILD="$(BUILD)" ./scripts/run_tests.sh $(SOURCES)

vars:
	@printf 'BUILD=%s\n' "$(BUILD)"
	@printf 'BINS=%s\n' "$(BINS)"
	@printf 'SOURCES=%s\n' "$(SOURCES)"
	@printf 'OBJS=%s\n' "$(OBJS)"
	@printf 'CC=%s\n' "$(CC)"
	@printf 'LD=%s\n' "$(LD)"
	@printf 'CFLAGS=%s\n' "$(CFLAGS)"
	@printf 'LDFLAGS=%s\n' "$(LDFLAGS)"

compile_commands.json:
	./scripts/compile_commands.sh

