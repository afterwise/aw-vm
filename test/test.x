
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

	a = vm_alloc(NULL, 64 * 1024, VM_RESERVE, NULL);
	vm_usage(&t, &r);
	printf("after reserve; total=%zx resident=%zx a=%p\n", t, r, a);

	vm_dealloc(a, 64 * 1024, 0, NULL);
	vm_usage(&t, &r);
	printf("after release; total=%zx resident=%zx a=%p\n", t, r, a);

	a = vm_alloc(NULL, 64 * 1024, VM_RESERVE, NULL);
	vm_usage(&t, &r);
	printf("after reserve; total=%zx resident=%zx a=%p\n", t, r, a);

	p = vm_alloc(a, 64 * 1024, 0, NULL);
	vm_usage(&t, &r);
	printf("after commit; total=%zx resident=%zx p=%p\n", t, r, p);
	assert(a == p);

	memset(p, 0, 64 * 1024);
	vm_usage(&t, &r);
	printf("after memset; total=%zx resident=%zx p=%p\n", t, r, p);

	vm_dealloc(p, 64 * 1024, VM_RESERVE, NULL);
	vm_usage(&t, &r);
	printf("after decommit; total=%zx resident=%zx\n", t, r);

	vm_dealloc(a, 64 * 1024, 0, NULL);
	vm_usage(&t, &r);
	printf("after release; total=%zx resident=%zx a=%p\n", t, r, a);

	vm_mapping_id_t id;
	a = vm_alloc(NULL, 64 * 1024, VM_MIRROR, &id);
	vm_usage(&t, &r);
	printf("alloc mirror; total=%zx resident=%zx a=%p\n", t, r, a);
	assert(a != NULL);

	for (int i = 0; i < 256; ++i)
		((unsigned char *) a)[i] = i;

	p = (unsigned char *) a + 64 * 1024;
	printf("check mirror; total=%zx resident=%zx p=%p\n", t, r, p);
	for (int i = 0; i < 256; ++i)
		assert(((unsigned char *) p)[i] == i);

	vm_dealloc(a, 64 * 1024, VM_MIRROR, id);
	vm_usage(&t, &r);
	printf("dealloc mirror; total=%zx resident=%zx\n", t, r);

	a = vm_alloc(NULL, 64 * 1024, 0, &id);
	vm_usage(&t, &r);
	printf("alloc w/o reserve; total=%zx resident=%zx a=%p\n", t, r, a);

	vm_dealloc(a, 64 * 1024, 0, id);
	vm_usage(&t, &r);
	printf("dealloc w/o reserve; total=%zx resident=%zx\n", t, r);

	printf("OK\n");
	return 0;
}

