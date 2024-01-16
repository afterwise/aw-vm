
export CFLAGS += -std=c99 -Wall -Wextra

test: test.o ../libaw-vm.a
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.x
	$(CC) $(CFLAGS) -I.. -xc -c $< -o $@

../libaw-vm.a:
	$(MAKE) -C..

.PHONY: clean
clean:
	rm -f test test.o

