/*
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Copyright (c) 2006 Maxim Yegorushkin <maxim.yegorushkin@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef MINHEAP_INTERNAL_H_INCLUDED_
#define MINHEAP_INTERNAL_H_INCLUDED_

#include "event2/event-config.h"
#include "evconfig-private.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "util-internal.h"
#include "mm-internal.h"

typedef struct min_heap
{
	struct event** p;	// struct event* 的数组
	size_t n, a;	// n为堆size, a为堆capacity
} min_heap_t;

static inline void	     min_heap_ctor_(min_heap_t* s);
static inline void	     min_heap_dtor_(min_heap_t* s);
static inline void	     min_heap_elem_init_(struct event* e);
static inline int	     min_heap_elt_is_top_(const struct event *e);
static inline int	     min_heap_empty_(min_heap_t* s);
static inline size_t	     min_heap_size_(min_heap_t* s);
static inline struct event*  min_heap_top_(min_heap_t* s);
static inline int	     min_heap_reserve_(min_heap_t* s, size_t n);
static inline int	     min_heap_push_(min_heap_t* s, struct event* e);
static inline struct event*  min_heap_pop_(min_heap_t* s);
static inline int	     min_heap_adjust_(min_heap_t *s, struct event* e);
static inline int	     min_heap_erase_(min_heap_t* s, struct event* e);
static inline void	     min_heap_shift_up_(min_heap_t* s, size_t hole_index, struct event* e);
static inline void	     min_heap_shift_up_unconditional_(min_heap_t* s, size_t hole_index, struct event* e);
static inline void	     min_heap_shift_down_(min_heap_t* s, size_t hole_index, struct event* e);

#define min_heap_elem_greater(a, b) \
	(evutil_timercmp(&(a)->ev_timeout, &(b)->ev_timeout, >))

void min_heap_ctor_(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }
void min_heap_dtor_(min_heap_t* s) { if (s->p) mm_free(s->p); }
void min_heap_elem_init_(struct event* e) { e->ev_timeout_pos.min_heap_idx = EV_SIZE_MAX; }
int min_heap_empty_(min_heap_t* s) { return 0 == s->n; }
size_t min_heap_size_(min_heap_t* s) { return s->n; }
struct event* min_heap_top_(min_heap_t* s) { return s->n ? *s->p : 0; }

// 插入堆, 放在最后一个然后上浮
int min_heap_push_(min_heap_t* s, struct event* e)
{
	if (min_heap_reserve_(s, s->n + 1))
		return -1;
	min_heap_shift_up_(s, s->n++, e);
	return 0;
}

// 弹出堆顶, 把最后一个元素放到堆顶下沉
// 注意需要把e->ev_timeout_pos.min_heap_idx标记清除为-1
struct event* min_heap_pop_(min_heap_t* s)
{
	if (s->n)
	{
		struct event* e = *s->p;
		min_heap_shift_down_(s, 0, s->p[--s->n]);
		e->ev_timeout_pos.min_heap_idx = EV_SIZE_MAX;
		return e;
	}
	return 0;
}

// 堆顶idx为0
int min_heap_elt_is_top_(const struct event *e)
{
	return e->ev_timeout_pos.min_heap_idx == 0;
}

// 删除堆中元素
int min_heap_erase_(min_heap_t* s, struct event* e)
{
	if (EV_SIZE_MAX != e->ev_timeout_pos.min_heap_idx)
	{
		struct event *last = s->p[--s->n];
		size_t parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
			min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, last);
		else
			min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, last);
		e->ev_timeout_pos.min_heap_idx = EV_SIZE_MAX;
		return 0;
	}
	return -1;
}

// 如果不在堆中先插入, 否则调整
int min_heap_adjust_(min_heap_t *s, struct event *e)
{
	if (EV_SIZE_MAX == e->ev_timeout_pos.min_heap_idx) {
		return min_heap_push_(s, e);
	} else {
		size_t parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;
		/* The position of e has changed; we shift it up or down
		 * as needed.  We can't need to do both. */
		if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], e))
			min_heap_shift_up_unconditional_(s, e->ev_timeout_pos.min_heap_idx, e);
		else
			min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, e);
		return 0;
	}
}

int min_heap_reserve_(min_heap_t* s, size_t n)
{
	// 增长策略, 如果capacity小于待分配n
	// 如果原来非0, capacity变为原来两倍
	// 如果原来为0, 变为8
	// 如果capacity还小于n则为n
	if (s->a < n)
	{
		struct event** p;
		size_t a = s->a ? s->a * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (struct event**)mm_realloc(s->p, a * sizeof *p)))
			return -1;
		s->p = p;
		s->a = a;
	}
	return 0;
}

// 不管实际先上浮一次, 是否继续再取决于父节点
// hole_index 为 e 开始的位置, 一般为e->ev_timeout_pos.min_heap_idx
void min_heap_shift_up_unconditional_(min_heap_t* s, size_t hole_index, struct event* e)
{
    size_t parent = (hole_index - 1) / 2;
    do
    {
	(s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
	hole_index = parent;
	parent = (hole_index - 1) / 2;
    } while (hole_index && min_heap_elem_greater(s->p[parent], e));
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

// 小根堆, 上浮直至小于父节点/到达根节点
// hole_index 为 e 开始的位置, 一般为e->ev_timeout_pos.min_heap_idx
void min_heap_shift_up_(min_heap_t* s, size_t hole_index, struct event* e)
{
    size_t parent = (hole_index - 1) / 2;
    while (hole_index && min_heap_elem_greater(s->p[parent], e))
    {
	(s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
	hole_index = parent;
	parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

// hole_index 为 e 开始的位置, 一般为e->ev_timeout_pos.min_heap_idx
void min_heap_shift_down_(min_heap_t* s, size_t hole_index, struct event* e)
{
    size_t min_child = 2 * (hole_index + 1);
    while (min_child <= s->n)
	{
	// 因为下标从0开始, 如果 min_child == s->n 说明, 只有左孩, 直接减1
	// 否则, 如果右孩大于左孩, 减1选左孩; 否则选右孩
	min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);
	// 如果e比孩子都小, 则不用下沉, break
	if (!(min_heap_elem_greater(e, s->p[min_child])))
	    break;
	(s->p[hole_index] = s->p[min_child])->ev_timeout_pos.min_heap_idx = hole_index;
	hole_index = min_child;
	min_child = 2 * (hole_index + 1);
	}
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

#endif /* MINHEAP_INTERNAL_H_INCLUDED_ */
