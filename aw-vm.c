
/*
   Copyright (c) 2014-2016 Malte Hildingsson, malte (at) afterwi.se

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

#ifndef _nofeatures
# if _WIN32
#  define WIN32_LEAN_AND_MEAN 1
# elif __linux__
#  define _BSD_SOURCE 1
#  define _DEFAULT_SOURCE 1
#  define _POSIX_C_SOURCE 200809L
#  define _SVID_SOURCE 1
# elif __APPLE__
#  define _DARWIN_C_SOURCE 1
# endif
#endif /* _nofeatures */

#include "aw-vm.h"

#if _WIN32
# include <psapi.h>
#endif

#if __linux__ || __APPLE__
# include <sys/mman.h>
# include <unistd.h>
#endif

#if __linux__
# include <stdio.h>
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

#include <stdlib.h>

size_t vm_page;
size_t vm_bigpage;

void vm_init(void) {
#if _WIN32
	SYSTEM_INFO si;
	TOKEN_PRIVILEGES tp;
	HANDLE tok;
	GetSystemInfo(&si);
	vm_page = si.dwPageSize;
	if (OpenProcessToken(INVALID_HANDLE_VALUE, TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &tok))
		if (LookupPrivilegeValue(NULL, "SeLockMemoryPrivilege", &tp.Privileges[0].Luid)) {
			tp.PrivilegeCount = 1;
			tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
			if (AdjustTokenPrivileges(tok, FALSE, &tp, 0, NULL, 0) &&
					GetLastError() == ERROR_SUCCESS)
				vm_bigpage = GetLargePageMinimum();
			CloseHandle(tok);
		}
#elif __linux__ || __APPLE__
	vm_page = sysconf(_SC_PAGESIZE);
	vm_bigpage = 2 * 1024 * 1024;
#elif __CELLOS_LV2__
	vm_page = 64 * 1024;
	vm_bigpage = 1 * 1024 * 1024;
#endif
}

void vm_usage(size_t *total, size_t *resident) {
#if _WIN32
	PROCESS_MEMORY_COUNTERS pmc;
	GetProcessMemoryInfo(INVALID_HANDLE_VALUE, &pmc, sizeof pmc);
	*total = pmc.PagefileUsage;
	*resident = pmc.WorkingSetSize;
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

void *vm_reserve(size_t n) {
	void *p;
#if _WIN32
	if ((p = VirtualAlloc(NULL, n, MEM_RESERVE, PAGE_NOACCESS)) != NULL)
		return p;
#elif __linux__ || __APPLE__
	if ((p = mmap(NULL, n, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0)) != MAP_FAILED)
		return p;
#elif __CELLOS_LV2__
	if (sys_mmapper_allocate_address(
			n, SYS_MEMORY_PAGE_SIZE_64K | SYS_MEMORY_ACCESS_RIGHT_ANY,
			vm_page, (uintptr_t *) &p) == 0)
		return p;
#endif
	return NULL;
}

void vm_release(void *p, size_t n) {
#if _WIN32
	VirtualFree(p, 0, MEM_RELEASE);
#elif __linux__ || __APPLE__
	munmap(p, n);
#elif __CELLOS_LV2__
	sys_mmapper_free_address(p);
#endif
}

void *vm_commit(void *p, size_t n, int flags) {
	int big = (flags & VM_BIGPAGES) != 0 && vm_bigpage != 0;
#if _WIN32
	if ((p = VirtualAlloc(p, n, MEM_COMMIT | (big ? MEM_LARGE_PAGES : 0), PAGE_READWRITE)) != NULL)
		return p;
#elif __linux__
	if ((p = mmap(
			p, n, PROT_READ | PROT_WRITE,
			MAP_PRIVATE | MAP_ANON | MAP_FIXED | (big ? MAP_HUGETLB : 0),
			-1, 0)) != MAP_FAILED)
		return p;
#elif __APPLE__
	if ((p = mmap(
			p, n, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON | MAP_FIXED,
			(big ? VM_FLAGS_SUPERPAGE_SIZE_2MB : -1), 0)) != MAP_FAILED)
		return p;
#elif __CELLOS_LV2__
	unsigned id;
	if (sys_mmapper_allocate_memory(
			n, (!big ? SYS_MEMORY_GRANULARITY_64K : SYS_MEMORY_GRANULARITY_1M),
			&id) == 0)
		if (sys_mmapper_map_memory((uintptr_t) p, id, SYS_MEMORY_PROT_READ_WRITE) == 0)
			return p;
#endif
	return NULL;
}

void vm_decommit(void *p, size_t n) {
#if _WIN32
	VirtualFree(p, n, MEM_DECOMMIT);
#elif __linux__ || __APPLE__
# if defined MADV_FREE
	madvise(p, n, MADV_FREE);
# elif defined MADV_DONTNEED
	madvise(p, n, MADV_DONTNEED);
# endif
#elif __CELLOS_LV2__
	unsigned id;
	sys_mmapper_unmap_memory((uintptr_t) p, &id);
	sys_mmapper_free_memory(id);
#endif
}

