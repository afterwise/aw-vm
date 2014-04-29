
#include "aw-vm.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	size_t size, rss;
	void *p;
	void *q;

	(void) argc;
	(void) argv;

	vm_init();

	vm_usage(&size, &rss);
	printf(" init: size=%lu rss=%lu\n", (unsigned long) size, (unsigned long) rss);

	assert(vm_base == vm_end);
	printf("  vm_maxsize=%lu\n", (unsigned long) vm_maxsize);
	printf("  vm_pagesize=%lu\n", (unsigned long) vm_pagesize);
	printf("  vm_largepagesize=%lu\n", (unsigned long) vm_largepagesize);
	printf("  vm_base=%p vm_end=%p\n", (void *) vm_base, (void *) vm_end);

	p = vm_increase(1, 0);
	printf("p=%p\n", p);

	vm_usage(&size, &rss);
	printf(" inc1: size=%lu rss=%lu\n", (unsigned long) size, (unsigned long) rss);
	printf("  vm_base=%p vm_end=%p\n", (void *) vm_base, (void *) vm_end);

	q = vm_increase(2, VM_LARGEPAGES);
	printf("q=%p\n", q);

	vm_usage(&size, &rss);
	printf(" inc2: size=%lu rss=%lu\n", (unsigned long) size, (unsigned long) rss);
	printf("  vm_base=%p vm_end=%p\n", (void *) vm_base, (void *) vm_end);

	vm_decrease(q, 2, VM_LARGEPAGES);

	vm_usage(&size, &rss);
	printf(" dec2: size=%lu rss=%lu\n", (unsigned long) size, (unsigned long) rss);
	printf("  vm_base=%p vm_end=%p\n", (void *) vm_base, (void *) vm_end);

	vm_decrease(q, 1, 0);

	vm_usage(&size, &rss);
	printf(" dec1: size=%lu rss=%lu\n", (unsigned long) size, (unsigned long) rss);
	printf("  vm_base=%p vm_end=%p\n", (void *) vm_base, (void *) vm_end);

	assert(vm_base == vm_end);

	return 0;
}

