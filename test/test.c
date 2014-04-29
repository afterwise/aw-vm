
#include "aw-vm.h"

#include <assert.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
	size_t total, resident;
	void *p;
	void *q;
	void *r;

	(void) argc;
	(void) argv;

	vm_init();

	assert(vm_base == vm_end);
	printf("              page = %08lx\n", (unsigned long) vm_page);
	printf("          big page = %08lx\n", (unsigned long) vm_bigpage);

	vm_usage(&total, &resident);
	printf("* %16p : total=%08lx resident=%08lx\n", NULL, (unsigned long) total, (unsigned long) resident);

	p = vm_increase(4, 0);
	*(int *) p = 0;
	vm_usage(&total, &resident);

	printf("+ %16p : total=%08lx resident=%08lx\n", p, (unsigned long) total, (unsigned long) resident);

	q = vm_increase(4, VM_BIGPAGES);
	*(int *) q = 0;
	vm_usage(&total, &resident);

	printf("+ %16p : total=%08lx resident=%08lx\n", q, (unsigned long) total, (unsigned long) resident);

	r = vm_increase(4, 0);
	*(int *) r = 0;
	vm_usage(&total, &resident);

	printf("+ %16p : total=%08lx resident=%08lx\n", r, (unsigned long) total, (unsigned long) resident);

	vm_decrease(r, 4, 0);
	vm_usage(&total, &resident);

	printf("- %16p : total=%08lx resident=%08lx\n", r, (unsigned long) total, (unsigned long) resident);

	vm_decrease(q, 4, VM_BIGPAGES);
	vm_usage(&total, &resident);

	printf("- %16p : total=%08lx resident=%08lx\n", q, (unsigned long) total, (unsigned long) resident);

	vm_decrease(p, 4, 0);
	vm_usage(&total, &resident);

	printf("- %16p : total=%08lx resident=%08lx\n", p, (unsigned long) total, (unsigned long) resident);

	assert(vm_base == vm_end);

	return 0;
}

