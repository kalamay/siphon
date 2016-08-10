#ifndef SIPHON_LIST_H
#define SIPHON_LIST_H

#include "common.h"

#include <stdbool.h>

typedef struct SpList SpList;

struct SpList {
	SpList *link[2];
};

__attribute__((unused)) static inline void
sp_list_init (SpList *list)
{
	list->link[0] = list->link[1] = list;
}

__attribute__((unused)) static inline void
sp_list_entry_clear (SpList *entry)
{
	entry->link[0] = NULL;
	entry->link[1] = NULL;
}

__attribute__((unused)) static inline bool
sp_list_is_empty (const SpList *list)
{
	return list->link[1] == list;
}

__attribute__((unused)) static inline bool
sp_list_is_singular (const SpList *list)
{
	return !sp_list_is_empty (list) && (list->link[0] == list->link[1]);
}

__attribute__((unused)) static inline bool
sp_list_is_added (const SpList *entry)
{
	return entry->link[0] != NULL;
}

__attribute__((unused)) static inline bool
sp_list_is_first (const SpList *list, const SpList *entry, SpOrder dir)
{
	return list->link[dir] == entry;
}

__attribute__((unused)) static inline SpList *
sp_list_first (const SpList *list, SpOrder dir)
{
	return list->link[dir] == list ? NULL : list->link[dir];
}

__attribute__((unused)) static inline SpList *
sp_list_next (const SpList *list, SpList *entry, SpOrder dir)
{
	return entry->link[dir] == list ? NULL : entry->link[dir];
}

__attribute__((unused)) static inline SpList *
sp_list_get (const SpList *list, int idx, SpOrder dir)
{
	SpList *entry = sp_list_first (list, dir);
	for (; idx > 0 && entry != NULL; idx--) {
		entry = sp_list_next (list, entry, dir);
	}
	return entry;
}

__attribute__((unused)) static inline bool
sp_list_has_next (const SpList *list, SpList *entry, SpOrder dir)
{
	return entry->link[dir] != list;
}

__attribute__((unused)) static inline void
sp_list_add (SpList *list, SpList *entry, SpOrder dir)
{
	SpList *link[2];
	link[dir] = list;
	link[!dir] = list->link[!dir];
	link[1]->link[0] = entry;
	entry->link[1] = link[1];
	entry->link[0] = link[0];
	link[0]->link[1] = entry;
}

__attribute__((unused)) static inline void
sp_list_del (SpList *entry)
{
	SpList *link[2] = { entry->link[0], entry->link[1] };
	link[0]->link[1] = link[1];
	link[1]->link[0] = link[0];
	sp_list_entry_clear (entry);
}

__attribute__((unused)) static inline SpList *
sp_list_pop (SpList *list, SpOrder dir)
{
	SpList *entry = sp_list_first (list, dir);
	if (entry != NULL) { sp_list_del (entry); }
	return entry;
}

__attribute__((unused)) static inline void
sp_list_copy_head (SpList *dst, SpList *src)
{
	dst->link[0] = src->link[0];
	dst->link[0]->link[1] = dst;
	dst->link[1] = src->link[1];
	dst->link[1]->link[0] = dst;
}

__attribute__((unused)) static inline void
sp_list_replace (SpList *dst, SpList *src)
{
	sp_list_copy_head (dst, src);
	sp_list_init (src);
}

__attribute__((unused)) static inline void
sp_list_swap (SpList *a, SpList *b)
{
	SpList tmp;
	sp_list_copy_head (&tmp, a);
	sp_list_copy_head (a, b);
	sp_list_copy_head (b, &tmp);
}

__attribute__((unused)) static inline void
sp_list_splice (SpList *list, SpList *other, SpOrder dir)
{
	if (!sp_list_is_empty (other)) {
		SpList *link[2];
		link[!dir] = list->link[!dir];
		link[dir] = list;
		link[!dir]->link[dir] = other->link[dir];
		other->link[dir]->link[!dir] = link[!dir];
		other->link[!dir]->link[dir] = link[dir];
		link[dir]->link[!dir] = other->link[!dir];
		sp_list_init (other);
	}
}

__attribute__((unused)) static inline void
sp_list_insert (SpList *entry, SpList *before)
{
	before->link[0]->link[1] = entry;
	entry->link[0] = before->link[0];
	entry->link[1] = before;
	before->link[0] = entry;
}

#define sp_list_each(list, var, dir)                                        \
	for (SpList *sp_sym(n) = (var = (list)->link[(dir)], var->link[(dir)]); \
	     var != (list);                                                     \
	     var = sp_sym(n), sp_sym(n) = sp_sym(n)->link[(dir)])

#endif

