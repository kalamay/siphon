CFLAGS := -std=c99 -Wall -Wextra -pedantic -Werror -O3
BUILD := build
DEF_INC := $(BUILD)/inc/def
DEF_BIN := $(BUILD)/bin/def
SSE_INC := $(BUILD)/inc/sse
SSE_BIN := $(BUILD)/bin/sse
TESTS := $(patsubst test/%.c,%,$(wildcard test/*.c))

all: test

test: test-def test-sse

test-def: $(TESTS:%=$(DEF_BIN)/%)
	@echo "*** default tests ***"
	@for name in $^; do ./$$name || exit; done

test-sse: $(TESTS:%=$(SSE_BIN)/%)
	@echo "*** sse4.2 tests ***"
	@for name in $^; do ./$$name || exit; done

-include $(wildcard $(DEF_INC)/*.d) $(wildcard $(SSE_INC)/*.d)

$(DEF_BIN)/%: test/%.c | $(DEF_INC) $(DEF_BIN)
	$(CC) $(CFLAGS) -MMD -MT $@ -MF $(DEF_INC)/$*.d $< -o $@

$(SSE_BIN)/%: test/%.c | $(SSE_INC) $(SSE_BIN)
	$(CC) $(CFLAGS) -msse4.2 -MMD -MT $@ -MF $(SSE_INC)/$*.d $< -o $@

$(DEF_INC) $(SSE_INC) $(DEF_BIN) $(SSE_BIN):
	@mkdir -p $@

clean:
	rm -rf build

.PHONY: clean test

