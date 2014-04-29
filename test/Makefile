
export CFLAGS += -Wall -Wextra

test: test.o ../libaw-vm.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -I.. -c $< -o $@

../libaw-vm.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

