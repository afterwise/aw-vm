
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

#ifndef AW_VM_H
#define AW_VM_H

#include <stddef.h>

#if !_MSC_VER
# include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

extern size_t vm_page;
extern size_t vm_bigpage;

void vm_init(void);
void vm_usage(size_t *total, size_t *resident);

enum {
	VM_BIGPAGES = 0x1
};

void *vm_reserve(size_t n);
void vm_release(void *p, size_t n);

void *vm_commit(void *p, size_t n, int flags);
void vm_decommit(void *p, size_t n);

/*
   Example snippet of how to use vm and lma to create a growable, contiguous area.
   Include aw-lma.h for this to compile.
 */

#define vlma_create(lma,n) do { \
		_lma_assert(((n) & (vm_page - 1)) == 0); \
		lma_init(lma, vm_reserve(n), n); \
		(lma)->high = (lma)->low; \
	} while (0)
#define vlma_destroy(lma) do { \
		vm_release((lma)->base, (lma)->end - (lma)->base); \
	} while (0)
#define vlma_alloc(lma,p,n) do { \
		while ((*(p) = lma_alloc_low(lma, n)) == NULL) { \
			if ((lma)->high >= (lma)->end) \
				abort(); \
			vm_commit((lma)->high, ((n) + (vm_page - 1)) & ~(vm_page - 1), 0); \
			(lma)->high += ((n) + (vm_page - 1)) & ~(vm_page - 1); \
		} \
	} while (0)
#define vlma_free(lma,p) do { \
		_lma_assert((lma_addr_t) (p) >= (lma)->base); \
		_lma_assert((lma_addr_t) (p) < (lma)->low); \
		lma_addr_t _vlma_free_p = \
			(lma_addr_t) (((uintptr_t) (lma)->low + (vm_page - 1)) & ~(vm_page - 1)); \
		vm_decommit(_vlma_free_p, (lma)->high - _vlma_free_p); \
		(lma)->high = _vlma_free_p; \
	} while (0)

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* AW_VM_H */

