/*
 * Copyright (C) 1998-2000 Netscape Communications Corporation.
 *
 * Other contributors:
 *   Nick Blievers <nickb@adacel.com.au>
 *   Jeff Hostetler <jeff@nerdone.com>
 *   Tom Rini <trini@kernel.crashing.org>
 *   Raffaele Sena <raff@netwinder.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Alternatively, the contents of this file may be used under the terms
 * of either the Mozilla Public License Version 1.1, found at
 * http://www.mozilla.org/MPL/ (the "MPL") or the GNU General Public
 * License Version 2.0, found at http://www.fsf.org/copyleft/gpl.html
 * (the "GPL"), in which case the provisions of the MPL or the GPL are
 * applicable instead of those above.  If you wish to allow use of your
 * version of this file only under the terms of one of those two
 * licenses (the MPL or the GPL) and not to allow others to use your
 * version of this file under the LGPL, indicate your decision by
 * deletingthe provisions above and replace them with the notice and
 * other provisions required by the MPL or the GPL, as the case may be.
 * If you do not delete the provisions above, a recipient may use your
 * version of this file under any of the LGPL, the MPL or the GPL.
 */

#ifndef ARENA_H
#define ARENA_H

#include <config-khtml.h>

// VG annotations for arenas disabled since still buggy
#if 0 && defined(HAVE_VALGRIND_MEMCHECK_H) && !defined(NDEBUG)

#include <valgrind/memcheck.h>
#define VALGRIND_SUPPORT

#else

#define VALGRIND_CREATE_MEMPOOL(base, redZone, zeroed)
#define VALGRIND_DESTROY_MEMPOOL(base)
#define VALGRIND_MEMPOOL_ALLOC(base, addr, size)
#define VALGRIND_MEMPOOL_FREE(base, addr)

#endif


#define ARENA_ALIGN_MASK 3

typedef unsigned long uword;

namespace khtml {

struct Arena {
    Arena* next; 	// next arena
    uword base;		// aligned base address
    uword limit;	// end of arena (1+last byte)
    uword avail;	// points to next available byte in arena
};

struct ArenaPool {
    Arena first;	// first arena in pool list.
    Arena* current;     // current arena.
    unsigned int arenasize;
    unsigned int largealloc; // threshold for fractional allocation strategy
    unsigned int cumul; // total bytes in pool.
    uword mask; 	// Mask (power-of-2 - 1)
};

void InitArenaPool(ArenaPool *pool, const char *name,
                   unsigned int size, unsigned int align);
void FinishArenaPool(ArenaPool *pool);
void FreeArenaPool(ArenaPool *pool);
void* ArenaAllocate(ArenaPool *pool, unsigned int nb);
void ArenaFinish(void);

#define ARENA_ALIGN(pool, n) (((uword)(n) + ARENA_ALIGN_MASK) & ~ARENA_ALIGN_MASK)
#define INIT_ARENA_POOL(pool, name, size) \
        InitArenaPool(pool, name, size, ARENA_ALIGN_MASK + 1)

#define ARENA_ALLOCATE(p, pool, nb) \
        Arena *_a = (pool)->current; \
        unsigned int _nb = ARENA_ALIGN(pool, nb); \
        uword _p = _a->avail; \
        uword _q = _p + _nb; \
        if (_q > _a->limit) \
            _p = (uword)ArenaAllocate(pool, _nb); \
        else { \
            VALGRIND_MEMPOOL_ALLOC(_a->base, _p, _nb); \
            _a->avail = _q; \
        } \
        p = (void *)_p;


#define ARENA_MARK(pool) ((void *) (pool)->current->avail)
#define UPTRDIFF(p,q) ((uword)(p) - (uword)(q))

#ifdef DEBUG
#define FREE_PATTERN 0xDA
#define CLEAR_UNUSED(a) (assert((a)->avail <= (a)->limit), \
                           memset((void*)(a)->avail, FREE_PATTERN, \
                            (a)->limit - (a)->avail))
#define CLEAR_ARENA(a)  memset((void*)(a), FREE_PATTERN, \
                            (a)->limit - (uword)(a))
#else
#define CLEAR_UNUSED(a)
#define CLEAR_ARENA(a)
#endif


} // namespace

#endif
