/*
 * This is actually a <linux/list.h>
 * Copyright (C) LINUX
 */

#ifndef ZFF_LIST_H
#define ZFF_LIST_H

#include "types.h"

/*
 * Simple doubly linked list implementation.
 *
 * Some of the internal functions ("__xxx") are useful when
 * manipulating whole lists rather than single entries, as
 * sometimes we already know the next/prev entries and we can
 * generate better code by using them directly rather than
 * using the generic single-entry routines.
 */

typedef struct zff_list {
	struct zff_list *next, *prev;
} zff_list_t;
#define list_head_t zff_list_t

#define ZLIST_INIT(name) { &(name), &(name) }

#define ZLIST(name) \
	zff_list_t name = ZLIST_INIT(name)

static inline void INIT_ZLIST(zff_list_t *list)
{
	list->next = list;
	list->prev = list;
}

/*
 * Insert a _new entry between two known consecutive entries.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __zff_list_add(zff_list_t *_new,
			      zff_list_t *prev,
			      zff_list_t *next)
{
	next->prev = _new;
	_new->next = next;
	_new->prev = prev;
	prev->next = _new;
}

/**
 * zlist_add - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it after
 *
 * Insert a _new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void zlist_add(zff_list_t *_new, zff_list_t *head)
{
	__zff_list_add(_new, head, head->next);
}


/**
 * zlist_add_tail - add a _new entry
 * @_new: _new entry to be added
 * @head: list head to add it before
 *
 * Insert a _new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void zlist_add_tail(zff_list_t *_new, zff_list_t *head)
{
	__zff_list_add(_new, head->prev, head);
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __zff_list_del(zff_list_t * prev, zff_list_t * next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * zlist_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: zlist_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void zlist_del(zff_list_t *entry)
{
	__zff_list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

/**
 * zlist_replace - replace old entry by _new one
 * @old : the element to be replaced
 * @_new : the _new element to insert
 *
 * If @old was empty, it will be overwritten.
 */
static inline void zlist_replace(zff_list_t *old,
				zff_list_t *_new)
{
	_new->next = old->next;
	_new->next->prev = _new;
	_new->prev = old->prev;
	_new->prev->next = _new;
}

static inline void zlist_replace_init(zff_list_t *old,
					zff_list_t *_new)
{
	zlist_replace(old, _new);
	INIT_ZLIST(old);
}

/**
 * zlist_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void zlist_del_init(zff_list_t *entry)
{
	__zff_list_del(entry->prev, entry->next);
	INIT_ZLIST(entry);
}

/**
 * zlist_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void zlist_move(zff_list_t *list, zff_list_t *head)
{
	__zff_list_del(list->prev, list->next);
	zlist_add(list, head);
}

/**
 * zlist_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void zlist_move_tail(zff_list_t *list,
				  zff_list_t *head)
{
	__zff_list_del(list->prev, list->next);
	zlist_add_tail(list, head);
}

/**
 * zlist_is_last - tests whether @list is the last entry in list @head
 * @list: the entry to test
 * @head: the head of the list
 */
static inline int zlist_is_last(const zff_list_t *list,
				const zff_list_t *head)
{
	return list->next == head;
}

/**
 * zlist_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int zlist_empty(const zff_list_t *head)
{
	return head->next == head;
}

/**
 * zlist_empty_careful - tests whether a list is empty and not being modified
 * @head: the list to test
 *
 * Description:
 * tests whether a list is empty _and_ checks that no other CPU might be
 * in the process of modifying either member (next or prev)
 *
 * NOTE: using zlist_empty_careful() without synchronization
 * can only be safe if the only activity that can happen
 * to the list entry is zlist_del_init(). Eg. it cannot be used
 * if another CPU could re-zlist_add() it.
 */
static inline int zlist_empty_careful(const zff_list_t *head)
{
	zff_list_t *next = head->next;
	return (next == head) && (next == head->prev);
}

/**
 * zlist_rotate_left - rotate the list to the left
 * @head: the head of the list
 */
static inline void zlist_rotate_left(zff_list_t *head)
{
	zff_list_t *first;

	if (!zlist_empty(head)) {
		first = head->next;
		zlist_move_tail(first, head);
	}
}

/**
 * zlist_is_singular - tests whether a list has just one entry.
 * @head: the list to test.
 */
static inline int zlist_is_singular(const zff_list_t *head)
{
	return !zlist_empty(head) && (head->next == head->prev);
}

static inline void __zff_list_cut_position(zff_list_t *list,
		zff_list_t *head, zff_list_t *entry)
{
	zff_list_t *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}

/**
 * zlist_cut_position - cut a list into two
 * @list: a _new list to add all removed entries
 * @head: a list with entries
 * @entry: an entry within head, could be the head itself
 *	and if so we won't cut the list
 *
 * This helper moves the initial part of @head, up to and
 * including @entry, from @head to @list. You should
 * pass on @entry an element you know is on @head. @list
 * should be an empty list or a list you do not care about
 * losing its data.
 *
 */
static inline void zlist_cut_position(zff_list_t *list,
		zff_list_t *head, zff_list_t *entry)
{
	if (zlist_empty(head))
		return;
	if (zlist_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_ZLIST(list);
	else
		__zff_list_cut_position(list, head, entry);
}

static inline void __zff_list_splice(const zff_list_t *list,
				 zff_list_t *prev,
				 zff_list_t *next)
{
	zff_list_t *first = list->next;
	zff_list_t *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * zlist_splice - join two lists, this is designed for stacks
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 */
static inline void zlist_splice(const zff_list_t *list,
				zff_list_t *head)
{
	if (!zlist_empty(list))
		__zff_list_splice(list, head, head->next);
}

/**
 * zlist_splice_tail - join two lists, each list being a queue
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 */
static inline void zlist_splice_tail(zff_list_t *list,
				zff_list_t *head)
{
	if (!zlist_empty(list))
		__zff_list_splice(list, head->prev, head);
}

/**
 * zlist_splice_init - join two lists and reinitialise the emptied list.
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void zlist_splice_init(zff_list_t *list,
				    zff_list_t *head)
{
	if (!zlist_empty(list)) {
		__zff_list_splice(list, head, head->next);
		INIT_ZLIST(list);
	}
}

/**
 * zlist_splice_tail_init - join two lists and reinitialise the emptied list
 * @list: the _new list to add.
 * @head: the place to add it in the first list.
 *
 * Each of the lists is a queue.
 * The list at @list is reinitialised
 */
static inline void zlist_splice_tail_init(zff_list_t *list,
					 zff_list_t *head)
{
	if (!zlist_empty(list)) {
		__zff_list_splice(list, head->prev, head);
		INIT_ZLIST(list);
	}
}

/**
 * zlist_entry - get the struct for this entry
 * @ptr:	the &zff_list_t pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define zlist_entry(ptr, type, member) \
	container_of(ptr, type, member)

/**
 * zlist_first_entry - get the first element from a list
 * @ptr:	the list head to take the element from.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 *
 * Note, that list is expected to be not empty.
 */
#define zlist_first_entry(ptr, type, member) \
	zlist_entry((ptr)->next, type, member)
#define zlist_last_entry(ptr, type, member) \
    zlist_entry((ptr)->prev, type, member)

/**
 * zlist_foreach	-	iterate over a list
 * @pos:	the &zff_list_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define zlist_foreach(pos, head) \
	for (pos = (head)->next; pos != (head); \
        	pos = pos->next)

/**
 * __zff_list_for_each	-	iterate over a list
 * @pos:	the &zff_list_t to use as a loop cursor.
 * @head:	the head for your list.
 *
 * This variant differs from zlist_foreach() in that it's the
 * simplest possible list iteration code, no prefetching is done.
 * Use this for code that knows the list to be very short (empty
 * or 1 entry) most of the time.
 */
#define __zff_list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * zlist_foreach_prev	-	iterate over a list backwards
 * @pos:	the &zff_list_t to use as a loop cursor.
 * @head:	the head for your list.
 */
#define zlist_foreach_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); \
        	pos = pos->prev)

/**
 * zlist_foreach_safe - iterate over a list safe against removal of list entry
 * @pos:	the &zff_list_t to use as a loop cursor.
 * @n:		another &zff_list_t to use as temporary storage
 * @head:	the head for your list.
 */
#define zlist_foreach_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

/**
 * zlist_foreach_prev_safe - iterate over a list backwards safe against removal of list entry
 * @pos:	the &zff_list_t to use as a loop cursor.
 * @n:		another &zff_list_t to use as temporary storage
 * @head:	the head for your list.
 */
#define zlist_foreach_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)

/**
 * zlist_foreach_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zlist_foreach_entry(pos, head, member)				\
	for (pos = zlist_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = zlist_entry(pos->member.next, typeof(*pos), member))

/**
 * zlist_foreach_entry_reverse - iterate backwards over list of given type.
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zlist_foreach_entry_reverse(pos, head, member)			\
	for (pos = zlist_entry((head)->prev, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = zlist_entry(pos->member.prev, typeof(*pos), member))

/**
 * zlist_prepare_entry - prepare a pos entry for use in zlist_foreach_entry_continue()
 * @pos:	the type * to use as a start point
 * @head:	the head of the list
 * @member:	the name of the list_struct within the struct.
 *
 * Prepares a pos entry for use as a start point in zlist_foreach_entry_continue().
 */
#define zlist_prepare_entry(pos, head, member) \
	((pos) ? : zlist_entry(head, typeof(*pos), member))

/**
 * zlist_foreach_entry_continue - continue iteration over list of given type
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Continue to iterate over list of given type, continuing after
 * the current position.
 */
#define zlist_foreach_entry_continue(pos, head, member) 		\
	for (pos = zlist_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = zlist_entry(pos->member.next, typeof(*pos), member))

/**
 * zlist_foreach_entry_continue_reverse - iterate backwards from the given point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Start to iterate over list of given type backwards, continuing after
 * the current position.
 */
#define zlist_foreach_entry_continue_reverse(pos, head, member)		\
	for (pos = zlist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head);	\
	     pos = zlist_entry(pos->member.prev, typeof(*pos), member))

/**
 * zlist_foreach_entry_from - iterate over list of given type from the current point
 * @pos:	the type * to use as a loop cursor.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing from current position.
 */
#define zlist_foreach_entry_from(pos, head, member) 			\
	for (; &pos->member != (head);	\
	     pos = zlist_entry(pos->member.next, typeof(*pos), member))

/**
 * zlist_foreach_entry_safe - iterate over list of given type safe against removal of list entry
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define zlist_foreach_entry_safe(pos, head, member) \
    typeof(pos) LINE_VAR(zlist_next); \
	for (pos = zlist_entry((head)->next, typeof(*pos), member),	\
        LINE_VAR(zlist_next) = zlist_entry(pos->member.next, typeof(*pos), member); \
        &pos->member != (head); \
        pos = LINE_VAR(zlist_next), \
        LINE_VAR(zlist_next) = zlist_entry(LINE_VAR(zlist_next)->member.next, typeof(*pos), member))

/**
 * zlist_foreach_entry_safe_continue - continue list iteration safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type, continuing after current point,
 * safe against removal of list entry.
 */
#define zlist_foreach_entry_safe_continue(pos, n, head, member) 		\
	for (pos = zlist_entry(pos->member.next, typeof(*pos), member), 		\
		n = zlist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = zlist_entry(n->member.next, typeof(*n), member))

/**
 * zlist_foreach_entry_safe_from - iterate over list from current point safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate over list of given type from current point, safe against
 * removal of list entry.
 */
#define zlist_foreach_entry_safe_from(pos, n, head, member) 			\
	for (n = zlist_entry(pos->member.next, typeof(*pos), member);		\
	     &pos->member != (head);						\
	     pos = n, n = zlist_entry(n->member.next, typeof(*n), member))

/**
 * zlist_foreach_entry_safe_reverse - iterate backwards over list safe against removal
 * @pos:	the type * to use as a loop cursor.
 * @n:		another type * to use as temporary storage
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 *
 * Iterate backwards over list of given type, safe against removal
 * of list entry.
 */
#define zlist_foreach_entry_safe_reverse(pos, n, head, member)		\
	for (pos = zlist_entry((head)->prev, typeof(*pos), member),	\
		n = zlist_entry(pos->member.prev, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = zlist_entry(n->member.prev, typeof(*n), member))

#endif // ZFF_LIST_H
