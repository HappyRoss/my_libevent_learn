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
#ifndef _MIN_HEAP_H_
#define _MIN_HEAP_H_

#include "event2/event-config.h"
#include "event2/event.h"
#include "event2/event_struct.h"
#include "event2/util.h"
#include "util-internal.h"
#include "mm-internal.h"

typedef struct min_heap
{
	struct event** p;//p指向最小堆成员数据(超时时间)
	unsigned n, a;//n为最小堆的大小-有数据个数；a为最小堆的可以存储数据的大小
} min_heap_t;

static inline void	     min_heap_ctor(min_heap_t* s);
static inline void	     min_heap_dtor(min_heap_t* s);
static inline void	     min_heap_elem_init(struct event* e);
static inline int	     min_heap_elt_is_top(const struct event *e);
static inline int	     min_heap_elem_greater(struct event *a, struct event *b);
static inline int	     min_heap_empty(min_heap_t* s);
static inline unsigned	     min_heap_size(min_heap_t* s);
static inline struct event*  min_heap_top(min_heap_t* s);
static inline int	     min_heap_reserve(min_heap_t* s, unsigned n);
static inline int	     min_heap_push(min_heap_t* s, struct event* e);
static inline struct event*  min_heap_pop(min_heap_t* s);
static inline int	     min_heap_erase(min_heap_t* s, struct event* e);
static inline void	     min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e);
static inline void	     min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e);

int min_heap_elem_greater(struct event *a, struct event *b)//min heap elem成员比较，比较的时间:tv_sec和tv_usec
{
	return evutil_timercmp(&a->ev_timeout, &b->ev_timeout, >);
}

void min_heap_ctor(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }//create 创建最小堆
void min_heap_dtor(min_heap_t* s) { if (s->p) mm_free(s->p); }//delete 删除最小堆
void min_heap_elem_init(struct event* e) { e->ev_timeout_pos.min_heap_idx = -1; }//min heap elem 成员初始化，超时最小堆的下标设置为-1
int min_heap_empty(min_heap_t* s) { return 0u == s->n; }//最小堆为空
unsigned min_heap_size(min_heap_t* s) { return s->n; }//最小堆的大小
struct event* min_heap_top(min_heap_t* s) { return s->n ? *s->p : 0; }//获取最小堆top:通过判断堆的大小是否为0来获取

int min_heap_push(min_heap_t* s, struct event* e)//push 入堆:插入e成员
{
	if (min_heap_reserve(s, s->n + 1))//调整最小堆可存储数据的大小
		return -1;
	min_heap_shift_up_(s, s->n++, e);//将e插入最小堆的最后面(s->n++)，需提升shift_up，以维护最小堆的性质
	return 0;
}

struct event* min_heap_pop(min_heap_t* s)//pop 出堆
{
    //判断最小堆是否为空
	if (s->n)
	{//非空
	    //获取最小堆(堆顶):最小值的数据成员
		struct event* e = *s->p;
        //将最小堆最后面的成员s->p[--s->n]放入到0的位置(堆顶)，并将成员个数减1，需调整位置已维护最小堆性质
		min_heap_shift_down_(s, 0u, s->p[--s->n]);
        //修改下标
		e->ev_timeout_pos.min_heap_idx = -1;
		return e;
	}
	return 0;
}

//判断成员e是否为最小堆堆顶成员，其中下标是否为0
int min_heap_elt_is_top(const struct event *e)
{
	return e->ev_timeout_pos.min_heap_idx == 0;
}

//从最小堆中删除成员e
int min_heap_erase(min_heap_t* s, struct event* e)
{
    //判断成员e的下标是否为-1，其中如果该成员时已经pop的话，则其下标已经被修改为-1
	if (-1 != e->ev_timeout_pos.min_heap_idx)
	{
		struct event *last = s->p[--s->n];//获取最小堆的最后一个成员，并修改最小堆成员个数大小(减1)
		unsigned parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;//计算成员e的parent的下标
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		//使用last代替e成员的位置
		//如果e下标大于0 且 parent的值比last的值要大，则需提升last成员
		if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
			min_heap_shift_up_(s, e->ev_timeout_pos.min_heap_idx, last);//提成last成员，其中hole_index设置为e成员的下标(使用last成员代替成员e)
		else
			min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, last);//下调e成员，其中hole_index设置为e成员的下标
		e->ev_timeout_pos.min_heap_idx = -1;//e成员的下标被修改为-1
		return 0;
	}
	return -1;
}

int min_heap_reserve(min_heap_t* s, unsigned n)//
{
    //判断: 当增加一个成员时，当前最小堆能否存储得下
	if (s->a < n)
	{
		struct event** p;
		unsigned a = s->a ? s->a * 2 : 8;//扩展最小堆的容量
		if (a < n)
			a = n;//如果扩展后还比n小，就直接将n赋值给a(容量)
		if (!(p = (struct event**)mm_realloc(s->p, a * sizeof *p)))
			return -1;
		s->p = p;
		s->a = a;
	}
	return 0;
}
//提升成员(hole_index, e),已维护最小堆性质
void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned parent = (hole_index - 1) / 2;//获取hole_index成员e的parent的下标
    //hole_index大于0 且 parent的值对e的值要大 需继续调整
    while (hole_index && min_heap_elem_greater(s->p[parent], e))
    {
        //1.perent位置的数据移到hole_index位置，并将原先perent数据的下标改为hole_index, 但e的数据现在还没有移到parent位置以及修改下标而是最后确定后在进行相应赋值和修改
	    (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
	    hole_index = parent;//更新hole_index为parent的下标
	    parent = (hole_index - 1) / 2;//计算新的hole_index对应的parent的下标
    }
    //最后确定了数据成员e位置:赋值，以及修改相应的位置的下标
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

//下调成员(hole_index, e),维护最小堆性质
void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned min_child = 2 * (hole_index + 1);//计算获取hole_index对应的child下标
    //min_child
    while (min_child <= s->n)
	{
	    //计算min_child  什么时候回出现min_child == s->n?
	    //影响min_child的计算:1.右孩子与左孩子比较；2.min_child == s->n
	    min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);

        //判断成员e是否比min_child成员要小 成立的话就直接break(因为其他的成员已经符合最小堆性质)
	    if (!(min_heap_elem_greater(e, s->p[min_child])))
	        break;
        //调整min_child成员的位置
	    (s->p[hole_index] = s->p[min_child])->ev_timeout_pos.min_heap_idx = hole_index;
	    hole_index = min_child;//使用min_child更新hole_index
	    min_child = 2 * (hole_index + 1);//重新计算min_child下标
	}
    //最后确定成员e的位置，并进行相应的调整
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

#endif /* _MIN_HEAP_H_ */
