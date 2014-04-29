
LIBSUF ?= .a

SOURCES := $(shell ls | grep -xE '(\w|\-)+\.c')
OBJECTS := $(patsubst %.c, %$(EXESUF).o, $(SOURCES))

PRODUCTS := libaw-vm$(EXESUF)$(LIBSUF)

.PHONY: all
all: $(PRODUCTS)

%$(LIBSUF): $(OBJECTS)
	$(AR) -r $@ $?

%$(EXESUF).o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

.PHONY: clean
clean:
	rm -fv *$(EXESUF).o *$(EXESUF)$(LIBSUF) | xargs echo --

