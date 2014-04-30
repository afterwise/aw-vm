
/*
   Copyright (c) 2014 Malte Hildingsson, malte (at) afterwi.se

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
 */

#include "aw-vm.h"

#if _WIN32
# include <psapi.h>
#endif

#if __linux__ || __APPLE__
# include <sys/mman.h>
# include <unistd.h>
#endif

#if __APPLE__
# include <mach/mach_init.h>
# include <mach/task.h>
# include <mach/vm_statistics.h>
#endif

#if __CELLOS_LV2__
# include <cell/gcm.h>
# include <sys/memory.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

size_t vm_page;
size_t vm_bigpage;

uintptr_t vm_base;
uintptr_t vm_end;

#if __CELLOS_LV2__
uintptr_t vm_rsxbase;
size_t vm_rsxsize;
#endif

void vm_init() {
	int err;
	(void) err;

#if _WIN32
	/* XXX: uncomment once mingw version of GetLargePageMinimum() is verified */

	SYSTEM_INFO si;
	/* TOKEN_PRIVILEGES tp;
	HANDLE tok; */

	GetSystemInfo(&si);
	vm_page = si.dwPageSize;

	/* if (OpenProcessToken((HANDLE) -1, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
		if (LookupPrivilegeValue(NULL, "SeLockMemoryPrivilege", &tp.Privileges[0].Luid)) {
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

			if (AdjustTokenPrivileges(tok, FALSE, &tp, 0, NULL, 0) &&
					GetLastError() == ERROR_SUCCESS)
				vm_bigpage = GetLargePageMinimum();

			CloseHandle(tok);
		} */
#elif __linux__ || __APPLE__
	vm_page = sysconf(_SC_PAGESIZE);
	vm_bigpage = 2 * 1024 * 1024;
#elif __CELLOS_LV2__
	vm_page = 64 * 1024;
	vm_bigpage = 1 * 1024 * 1024;

	if ((err = sys_mmapper_allocate_address(
# ifndef NDEBUG
			512 * 1024 * 1024,
# else
			256 * 1024 * 1024,
# endif
			SYS_MEMORY_PAGE_SIZE_64K | SYS_MEMORY_ACCESS_RIGHT_ANY,
			256 * 1024 * 1024, &vm_base)) != 0)
		fprintf(stderr, "sys_mmapper_allocate_address: %d\n", err), abort();

	vm_end = vm_base;

# ifndef NDEBUG
	if ((err = cellGcmInitSystemMode(CELL_GCM_SYSTEM_MODE_IOMAP_512MB)) != 0)
		fprintf(stderr, "cellGcmInitSystemMode: %d\n", err), abort();
# endif

	if ((err = cellGcmMapLocalMemory((void **) &vm_rsxbase, &vm_rsxsize)) != 0)
		fprintf(stderr, "cellGcmMapLocalMemory: %d\n", err), abort();
#endif
}

void vm_usage(size_t *total, size_t *resident) {
#if _WIN32
	/* XXX: uncomment once mingw version of GetProcessMemoryInfo() is verified */
	*total = 0;
	*resident = 0;

	/* PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo((HANDLE) -1, &pmc, sizeof pmc);

	*total = pmc.WorkingSetSize;
	*resident = pmc.WorkingSetSize; */
#elif __linux__
	unsigned long long sz = 0, rp = 0;
	FILE *fd;

	if ((fd = fopen("/proc/self/stat", "rb")) != NULL) {
		fscanf(
			fd,
			"%*u (%*256[^)]) %*c %*u %*u %*u %*u %*u %*u %*u %*u %*u %*u"
			"%*u %*u %*u %*u %*u %*u %*u %*u %*u %llu %llu",
			&sz, &rp);
		fclose(fd);
	}

	*total = sz;
	*resident = rp * vm_page;
#elif __APPLE__
	struct task_basic_info info;
	mach_msg_type_number_t n = TASK_BASIC_INFO_COUNT;
	task_t t;

	if (task_for_pid(current_task(), getpid(), &t) == KERN_SUCCESS)
		task_info(t, TASK_BASIC_INFO, (task_info_t) &info, &n);

	*total = info.virtual_size;
	*resident = info.resident_size;
#elif __CELLOS_LV2__
	sys_memory_info_t info;
	sys_memory_get_user_memory_size(&info);

	*total = info.total_user_memory;
	*resident = info.total_user_memory - info.available_user_memory;
#endif
}

void *vm_increase(size_t n, int flags) {
	void *p = NULL;
	size_t z = vm_page;
	int err;

	(void) err;

	if ((flags & VM_BIGPAGES) && vm_bigpage)
		z = vm_bigpage;

	n = (n + (z - 1)) & ~(z - 1);

#if _WIN32
	if ((p = VirtualAlloc(
			NULL, n, MEM_RESERVE | MEM_COMMIT |
				/* XXX: uncomment once MEM_LARGE_PAGES is confirmed in mingw */
				(/*(flags & VM_BIGPAGES) && vm_bigpage ? MEM_LARGE_PAGES :*/ 0),
			PAGE_READWRITE)) == NULL)
		fprintf(stderr, "VirtualAlloc: %ld\n", GetLastError()), abort();
#elif __linux__
	if ((p = mmap(
			NULL, n, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE |
				(flags & VM_BIGPAGES ? MAP_HUGETLB : 0), -1, 0)) == MAP_FAILED)
		fprintf(stderr, "mmap: %d\n", errno), abort();
#elif __APPLE__
	if ((p = mmap(
			NULL, n, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
			/* XXX: uncomment once superpages work */
			(/*flags & VM_BIGPAGES ? VM_FLAGS_SUPERPAGE_SIZE_2MB :*/ -1), 0)) == MAP_FAILED)
		fprintf(stderr, "mmap: %d\n", errno), abort();
#elif __CELLOS_LV2__
	unsigned id;

	if ((err = sys_mmapper_allocate_memory(
			n, (!(flags & VM_BIGPAGES) ?
				SYS_MEMORY_GRANULARITY_64K : SYS_MEMORY_GRANULARITY_1M),
			&id)) != 0)
		fprintf(stderr, "sys_mmapper_allocate_memory: %d\n", err), abort();

	if ((err = sys_mmapper_map_memory(vm_end, id, SYS_MEMORY_PROT_READ_WRITE)) != 0)
		fprintf(stderr, "sys_mmapper_map_memory: %d\n", err), abort();

	p = (void *) vm_end;
#endif

	vm_end += n;
	return p;
}

void vm_decrease(void *p, size_t n, int flags) {
	size_t z = vm_page;
	int err;

	(void) err;

	if ((flags & VM_BIGPAGES) && vm_bigpage)
		z = vm_bigpage;

	n = (n + (z - 1)) & ~(z - 1);

#if _WIN32
	if (!VirtualFree(p, 0, MEM_RELEASE))
		fprintf(stderr, "VirtualFree: %ld\n", GetLastError()), abort();
#elif __linux__ || __APPLE__
	if (munmap(p, n) != 0)
		fprintf(stderr, "munmap: %d\n", errno), abort();
#elif __CELLOS_LV2__
	unsigned id;

	if ((err = sys_mmapper_unmap_memory((uintptr_t) p, &id)) != 0)
		fprintf(stderr, "sys_mmapper_unmap_memory: %d\n", err), abort();

	if ((err = sys_mmapper_free_memory(id)) != 0)
		fprintf(stderr, "sys_mmapper_free_memory: %d\n", err), abort();
#endif

	vm_end -= n;
}

