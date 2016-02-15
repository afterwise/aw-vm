
#include "aw-vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
	(void) argc;
	(void) argv;

	void *a, *p;
	size_t t, r;

	vm_init();
	printf("vm_page=%zx\n", vm_page);

	vm_usage(&t, &r);
	printf("after init; total=%zx resident=%zx\n", t, r);

	a = vm_reserve(64 * 1024);
	vm_usage(&t, &r);
	printf("after reserve; total=%zx resident=%zx a=%p\n", t, r, a);

	vm_release(a, 64 * 1024);
	vm_usage(&t, &r);
	printf("after release; total=%zx resident=%zx a=%p\n", t, r, a);

	a = vm_reserve(64 * 1024);
	vm_usage(&t, &r);
	printf("after reserve; total=%zx resident=%zx a=%p\n", t, r, a);

	p = vm_commit(a, 64 * 1024, 0);
	vm_usage(&t, &r);
	printf("after commit; total=%zx resident=%zx p=%p\n", t, r, p);
	assert(a == p);

	memset(p, 0, 64 * 1024);
	vm_usage(&t, &r);
	printf("after memset; total=%zx resident=%zx p=%p\n", t, r, p);

	vm_decommit(p, 64 * 1024);
	vm_usage(&t, &r);
	printf("after decommit; total=%zx resident=%zx\n", t, r);

	vm_release(a, 64 * 1024);
	vm_usage(&t, &r);
	printf("after release; total=%zx resident=%zx a=%p\n", t, r, a);

	return 0;
}

