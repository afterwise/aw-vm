
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

#ifndef AW_VM_H
#define AW_VM_H

#include <stddef.h>

#if !_MSC_VER
# include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern unsigned long long vm_maxsize;
extern size_t vm_pagesize;
extern size_t vm_largepagesize;

extern uintptr_t vm_base;
extern uintptr_t vm_end;

extern uintptr_t vm_auxbase;
extern size_t vm_auxsize;

void vm_init();
void vm_usage(size_t *total, size_t *resident);

/*
   The intention here is to grow the virtual memory space linearly.
   Use these functions to create scopes. They are inherently not
   considered to be thread safe.
 */

enum {
	VM_LARGEPAGES = 0x1
};

void *vm_increase(size_t n, int flags);
void vm_decrease(void *p, size_t n, int flags);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_VM_H */

