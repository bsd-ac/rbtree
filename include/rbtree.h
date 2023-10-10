/*
 * Copyright (c) 2016 David Gwynne <dlg@openbsd.org>
 * Copyright (c) 2023 Aisha Tammy <aisha@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef	_SYS_RBTREE_H_
#define	_SYS_RBTREE_H_

#include <stddef.h>
#include <sys/cdefs.h>
#include <stdlib.h>
#include <stdint.h>

struct rb_tree;
struct rb_entry;

struct rb_type {
	int		(*t_compare)(const void *, const void *);
	int		(*t_augment)(struct rb_tree *, void *);
	unsigned int	  t_offset;	/* offset of rb_entry in type */
};

/*
 * Allow choosing an implementation without the parent pointer.
 * The advantage is a much smaller tree representation, faster lookup
 * times but at the cost of a slight increase in insert/removal times.
 */
#ifdef RBT_SMALL

struct rb_entry {
	/* left, right */
	struct rb_entry		*child[2];
};


#ifndef RB_MAX_HEIGHT
#define RB_MAX_HEIGHT		127
#endif

struct rb_tree {
	struct rb_entry		*root;
	struct rb_type		*options;
	struct rb_entry		*stack[RB_MAX_HEIGHT];
	size_t			 top;
};

#define RBT_INITIALIZER(_head)	{ NULL, NULL, { NULL }, 0 }

#else

struct rb_entry {
	/* left, right, parent */
	struct rb_entry		*child[3];
};

struct rb_tree {
	struct rb_entry		*root;
	struct rb_type		*options;
};

#define RBT_INITIALIZER(_head)	{ NULL, NULL }

#endif

#define RBT_HEAD(_name, _type)						\
struct _name {								\
	struct rb_tree		*rbh_root;				\
}

#define RBT_ENTRY(_type)	struct rb_entry

void	 rb_init(struct rb_tree *);
int	 rb_empty(struct rb_tree *);
int 	 rb_rank(struct rb_tree *);
void	*rb_root(struct rb_tree *);
void	*rb_min(struct rb_tree *);
void	*rb_max(struct rb_tree *);
void	*rb_insert(struct rb_tree *, void *);
void	*rb_insert_next(struct rb_tree *, void *, void *);
void	*rb_insert_prev(struct rb_tree *, void *, void *);
void	*rb_remove(struct rb_tree *, void *);
void	*rb_find(struct rb_tree *, void *);
void	*rb_nfind(struct rb_tree *, void *);
void	*rb_pfind(struct rb_tree *, void *);
void	*rb_next(struct rb_tree *, void *);
void	*rb_prev(struct rb_tree *, void *);
void	*rb_left(struct rb_tree *, void *);
void	*rb_right(struct rb_tree *, void *);
void	*rb_parent(struct rb_tree *, void *);
void	 rb_set_left(struct rb_tree *, void *, void *);
void	 rb_set_right(struct rb_tree *, void *, void *);
void	 rb_set_parent(struct rb_tree *, void *, void *);
void	 rb_poison(struct rb_tree *, void *, unsigned long);
int	 rb_check(const struct rb_tree *, void *, unsigned long);

/*
#define RBT_PROTOTYPE(_name, _type, _field, _cmp)			\
extern const struct rb_type *const _name##_RBT_TYPE;			\
									\
__unused static inline void						\
_name##_RBT_INIT(struct _name *head)					\
{									\
	rb_init(&head->rbh_root);					\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_INSERT(struct _name *head, struct _type *elm)		\
{									\
	return rb_insert(_name##_RBT_TYPE, &head->rbh_root, elm);	\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_REMOVE(struct _name *head, struct _type *elm)		\
{									\
	return rb_remove(_name##_RBT_TYPE, &head->rbh_root, elm);	\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_FIND(struct _name *head, const struct _type *key)		\
{									\
	return rb_find(_name##_RBT_TYPE, &head->rbh_root, key);		\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_NFIND(struct _name *head, const struct _type *key)		\
{									\
	return rb_nfind(_name##_RBT_TYPE, &head->rbh_root, key);	\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_PFIND(struct _name *head, const struct _type *key)		\
{									\
	return rb_pfind(_name##_RBT_TYPE, &head->rbh_root, key);	\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_ROOT(struct _name *head)					\
{									\
	return rb_root(_name##_RBT_TYPE, &head->rbh_root);		\
}									\
									\
__unused static inline int						\
_name##_RBT_EMPTY(struct _name *head)					\
{									\
	return rb_empty(&head->rbh_root);				\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_MIN(struct _name *head)					\
{									\
	return rb_min(_name##_RBT_TYPE, &head->rbh_root);		\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_MAX(struct _name *head)					\
{									\
	return rb_max(_name##_RBT_TYPE, &head->rbh_root);		\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_NEXT(struct _type *elm)					\
{									\
	return rb_next(_name##_RBT_TYPE, elm);				\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_PREV(struct _type *elm)					\
{									\
	return rb_prev(_name##_RBT_TYPE, elm);				\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_LEFT(struct _type *elm)					\
{									\
	return rb_left(_name##_RBT_TYPE, elm);				\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_RIGHT(struct _type *elm)					\
{									\
	return rb_right(_name##_RBT_TYPE, elm);				\
}									\
									\
__unused static inline struct _type *					\
_name##_RBT_PARENT(struct _type *elm)					\
{									\
	return rb_parent(_name##_RBT_TYPE, elm);			\
}									\
									\
__unused static inline void						\
_name##_RBT_SET_LEFT(struct _type *elm, struct _type *left)		\
{									\
	rb_set_left(_name##_RBT_TYPE, elm, left);			\
}									\
									\
__unused static inline void						\
_name##_RBT_SET_RIGHT(struct _type *elm, struct _type *right)		\
{									\
	rb_set_right(_name##_RBT_TYPE, elm, right);			\
}									\
									\
__unused static inline void						\
_name##_RBT_SET_PARENT(struct _type *elm, struct _type *parent)		\
{									\
	rb_set_parent(_name##_RBT_TYPE, elm, parent);			\
}									\
									\
__unused static inline void						\
_name##_RBT_POISON(struct _type *elm, unsigned long poison)		\
{									\
	rb_poison(_name##_RBT_TYPE, elm, poison);			\
}									\
									\
__unused static inline int						\
_name##_RBT_CHECK(struct _type *elm, unsigned long poison)		\
{									\
	return rb_check(_name##_RBT_TYPE, elm, poison);			\
}

#define RBT_GENERATE_INTERNAL(_name, _type, _field, _cmp, _aug)		\
static int								\
_name##_RBT_COMPARE(const void *lptr, const void *rptr)			\
{									\
	const struct _type *l = lptr, *r = rptr;			\
	return _cmp(l, r);						\
}									\
static const struct rb_type _name##_RBT_INFO = {			\
	_name##_RBT_COMPARE,						\
	_aug,								\
	offsetof(struct _type, _field),					\
};									\
const struct rb_type *const _name##_RBT_TYPE = &_name##_RBT_INFO

#define RBT_GENERATE_AUGMENT(_name, _type, _field, _cmp, _aug)		\
static int								\
_name##_RBT_AUGMENT(void *ptr)						\
{									\
	struct _type *p = ptr;						\
	return _aug(p);							\
}									\
RBT_GENERATE_INTERNAL(_name, _type, _field, _cmp, _name##_RBT_AUGMENT)

#define RBT_GENERATE(_name, _type, _field, _cmp)			\
    RBT_GENERATE_INTERNAL(_name, _type, _field, _cmp, NULL)

#define RBT_INIT(_name, _head)		_name##_RBT_INIT(_head)
#define RBT_INSERT(_name, _head, _elm)	_name##_RBT_INSERT(_head, _elm)
#define RBT_REMOVE(_name, _head, _elm)	_name##_RBT_REMOVE(_head, _elm)
#define RBT_FIND(_name, _head, _key)	_name##_RBT_FIND(_head, _key)
#define RBT_NFIND(_name, _head, _key)	_name##_RBT_NFIND(_head, _key)
#define RBT_PFIND(_name, _head, _key)	_name##_RBT_PFIND(_head, _key)
#define RBT_ROOT(_name, _head)		_name##_RBT_ROOT(_head)
#define RBT_EMPTY(_name, _head)		_name##_RBT_EMPTY(_head)
#define RBT_MIN(_name, _head)		_name##_RBT_MIN(_head)
#define RBT_MAX(_name, _head)		_name##_RBT_MAX(_head)
#define RBT_NEXT(_name, _head, _elm)	_name##_RBT_NEXT(_elm)
#define RBT_PREV(_name, _elm)		_name##_RBT_PREV(_elm)
#define RBT_LEFT(_name, _elm)		_name##_RBT_LEFT(_elm)
#define RBT_RIGHT(_name, _elm)		_name##_RBT_RIGHT(_elm)
#define RBT_PARENT(_name, _elm)		_name##_RBT_PARENT(_elm)
#define RBT_SET_LEFT(_name, _elm, _l)	_name##_RBT_SET_LEFT(_elm, _l)
#define RBT_SET_RIGHT(_name, _elm, _r)	_name##_RBT_SET_RIGHT(_elm, _r)
#define RBT_SET_PARENT(_name, _elm, _p)	_name##_RBT_SET_PARENT(_elm, _p)
#define RBT_POISON(_name, _elm, _p)	_name##_RBT_POISON(_elm, _p)
#define RBT_CHECK(_name, _elm, _p)	_name##_RBT_CHECK(_elm, _p)


#define RBT_FOREACH(_e, _name, _head)					\
	for ((_e) = RBT_MIN(_name, (_head));				\
	     (_e) != NULL;						\
	     (_e) = RBT_NEXT(_name, (_e)))

#define RBT_FOREACH_REVERSE(_e, _name, _head)				\
	for ((_e) = RBT_MAX(_name, (_head));				\
	     (_e) != NULL;						\
	     (_e) = RBT_PREV(_name, (_e)))

#define RBT_FOREACH_SAFE(_e, _name, _head, _n)				\
	for ((_e) = RBT_MIN(_name, (_head));				\
	     (_e) != NULL && ((_n) = RBT_NEXT(_name, (_e)), 1);		\
	     (_e) = (_n))

#define RBT_FOREACH_REVERSE_SAFE(_e, _name, _head, _n)			\
	for ((_e) = RBT_MAX(_name, (_head));				\
	     (_e) != NULL && ((_n) = RBT_PREV(_name, (_e)), 1);		\
	     (_e) = (_n))
*/

#endif	/* _SYS_RBTREE_H_ */
