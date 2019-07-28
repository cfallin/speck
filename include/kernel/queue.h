/*
 * include/kernel/queue.h
 */

#ifndef _KERNEL_QUEUE_H_
#define _KERNEL_QUEUE_H_

#define queue_insert_generic(q,i,pre) {\
	(i)->  pre ## queue = (q); \
	(i)->  pre ## next = (void *)0; \
	(i)->  pre ## prev = (q)->tail; \
	if((q)->tail) \
		(q)->tail->  pre ## next = (i); \
	else \
		(q)->head = (i); \
	(q)->tail = (i); \
}

#define queue_remove_generic(i,pre) {\
	if((i)-> pre ## queue) \
	{ \
		(i)-> pre ## queue->head = (i)->pre ## next; \
		if((i)-> pre ## next) \
			(i)-> pre ## next->  pre ## prev = (i)->  pre ## prev; \
		else \
			(i)-> pre ## queue->tail = (i)->  pre ## prev; \
		if((i)-> pre ## prev) \
			(i)-> pre ## prev->  pre ## next = (i)->  pre ## next; \
		else \
			(i)-> pre ## queue->head = (i)->  pre ## next; \
		(i)-> pre ## queue = (void *)0; \
	} \
}

#define queue_insert(q,i) \
	queue_insert_generic(q,i,)

#define queue_remove(i) \
	queue_remove_generic(i,)

#endif
