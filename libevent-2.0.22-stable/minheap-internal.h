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
	struct event** p;//pָ����С�ѳ�Ա����(��ʱʱ��)
	unsigned n, a;//nΪ��С�ѵĴ�С-�����ݸ�����aΪ��С�ѵĿ��Դ洢���ݵĴ�С
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

int min_heap_elem_greater(struct event *a, struct event *b)//min heap elem��Ա�Ƚϣ��Ƚϵ�ʱ��:tv_sec��tv_usec
{
	return evutil_timercmp(&a->ev_timeout, &b->ev_timeout, >);
}

void min_heap_ctor(min_heap_t* s) { s->p = 0; s->n = 0; s->a = 0; }//create ������С��
void min_heap_dtor(min_heap_t* s) { if (s->p) mm_free(s->p); }//delete ɾ����С��
void min_heap_elem_init(struct event* e) { e->ev_timeout_pos.min_heap_idx = -1; }//min heap elem ��Ա��ʼ������ʱ��С�ѵ��±�����Ϊ-1
int min_heap_empty(min_heap_t* s) { return 0u == s->n; }//��С��Ϊ��
unsigned min_heap_size(min_heap_t* s) { return s->n; }//��С�ѵĴ�С
struct event* min_heap_top(min_heap_t* s) { return s->n ? *s->p : 0; }//��ȡ��С��top:ͨ���ж϶ѵĴ�С�Ƿ�Ϊ0����ȡ

int min_heap_push(min_heap_t* s, struct event* e)//push ���:����e��Ա
{
	if (min_heap_reserve(s, s->n + 1))//������С�ѿɴ洢���ݵĴ�С
		return -1;
	min_heap_shift_up_(s, s->n++, e);//��e������С�ѵ������(s->n++)��������shift_up����ά����С�ѵ�����
	return 0;
}

struct event* min_heap_pop(min_heap_t* s)//pop ����
{
    //�ж���С���Ƿ�Ϊ��
	if (s->n)
	{//�ǿ�
	    //��ȡ��С��(�Ѷ�):��Сֵ�����ݳ�Ա
		struct event* e = *s->p;
        //����С�������ĳ�Աs->p[--s->n]���뵽0��λ��(�Ѷ�)��������Ա������1�������λ����ά����С������
		min_heap_shift_down_(s, 0u, s->p[--s->n]);
        //�޸��±�
		e->ev_timeout_pos.min_heap_idx = -1;
		return e;
	}
	return 0;
}

//�жϳ�Աe�Ƿ�Ϊ��С�ѶѶ���Ա�������±��Ƿ�Ϊ0
int min_heap_elt_is_top(const struct event *e)
{
	return e->ev_timeout_pos.min_heap_idx == 0;
}

//����С����ɾ����Աe
int min_heap_erase(min_heap_t* s, struct event* e)
{
    //�жϳ�Աe���±��Ƿ�Ϊ-1����������ó�Աʱ�Ѿ�pop�Ļ��������±��Ѿ����޸�Ϊ-1
	if (-1 != e->ev_timeout_pos.min_heap_idx)
	{
		struct event *last = s->p[--s->n];//��ȡ��С�ѵ����һ����Ա�����޸���С�ѳ�Ա������С(��1)
		unsigned parent = (e->ev_timeout_pos.min_heap_idx - 1) / 2;//�����Աe��parent���±�
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		//ʹ��last����e��Ա��λ��
		//���e�±����0 �� parent��ֵ��last��ֵҪ����������last��Ա
		if (e->ev_timeout_pos.min_heap_idx > 0 && min_heap_elem_greater(s->p[parent], last))
			min_heap_shift_up_(s, e->ev_timeout_pos.min_heap_idx, last);//���last��Ա������hole_index����Ϊe��Ա���±�(ʹ��last��Ա�����Աe)
		else
			min_heap_shift_down_(s, e->ev_timeout_pos.min_heap_idx, last);//�µ�e��Ա������hole_index����Ϊe��Ա���±�
		e->ev_timeout_pos.min_heap_idx = -1;//e��Ա���±걻�޸�Ϊ-1
		return 0;
	}
	return -1;
}

int min_heap_reserve(min_heap_t* s, unsigned n)//
{
    //�ж�: ������һ����Աʱ����ǰ��С���ܷ�洢����
	if (s->a < n)
	{
		struct event** p;
		unsigned a = s->a ? s->a * 2 : 8;//��չ��С�ѵ�����
		if (a < n)
			a = n;//�����չ�󻹱�nС����ֱ�ӽ�n��ֵ��a(����)
		if (!(p = (struct event**)mm_realloc(s->p, a * sizeof *p)))
			return -1;
		s->p = p;
		s->a = a;
	}
	return 0;
}
//������Ա(hole_index, e),��ά����С������
void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned parent = (hole_index - 1) / 2;//��ȡhole_index��Աe��parent���±�
    //hole_index����0 �� parent��ֵ��e��ֵҪ�� ���������
    while (hole_index && min_heap_elem_greater(s->p[parent], e))
    {
        //1.perentλ�õ������Ƶ�hole_indexλ�ã�����ԭ��perent���ݵ��±��Ϊhole_index, ��e���������ڻ�û���Ƶ�parentλ���Լ��޸��±�������ȷ�����ڽ�����Ӧ��ֵ���޸�
	    (s->p[hole_index] = s->p[parent])->ev_timeout_pos.min_heap_idx = hole_index;
	    hole_index = parent;//����hole_indexΪparent���±�
	    parent = (hole_index - 1) / 2;//�����µ�hole_index��Ӧ��parent���±�
    }
    //���ȷ�������ݳ�Աeλ��:��ֵ���Լ��޸���Ӧ��λ�õ��±�
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

//�µ���Ա(hole_index, e),ά����С������
void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, struct event* e)
{
    unsigned min_child = 2 * (hole_index + 1);//�����ȡhole_index��Ӧ��child�±�
    //min_child
    while (min_child <= s->n)
	{
	    //����min_child  ʲôʱ��س���min_child == s->n?
	    //Ӱ��min_child�ļ���:1.�Һ��������ӱȽϣ�2.min_child == s->n
	    min_child -= min_child == s->n || min_heap_elem_greater(s->p[min_child], s->p[min_child - 1]);

        //�жϳ�Աe�Ƿ��min_child��ԱҪС �����Ļ���ֱ��break(��Ϊ�����ĳ�Ա�Ѿ�������С������)
	    if (!(min_heap_elem_greater(e, s->p[min_child])))
	        break;
        //����min_child��Ա��λ��
	    (s->p[hole_index] = s->p[min_child])->ev_timeout_pos.min_heap_idx = hole_index;
	    hole_index = min_child;//ʹ��min_child����hole_index
	    min_child = 2 * (hole_index + 1);//���¼���min_child�±�
	}
    //���ȷ����Աe��λ�ã���������Ӧ�ĵ���
    (s->p[hole_index] = e)->ev_timeout_pos.min_heap_idx = hole_index;
}

#endif /* _MIN_HEAP_H_ */
