/*	$OpenBSD: subr_tree.c,v 1.10 2018/10/09 08:28:43 dlg Exp $ */

/*
 * Copyright 2002 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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

#include "rbtree.h"
#include <stdio.h>
#include <assert.h>

#ifndef __uintptr_t
#define __uintptr_t unsigned long long
#endif

/*
 * Debug macros
 */
#if defined(KERNEL) && defined(DIAGNOSTIC)
#define _RBT_ASSERT(x)		KASSERT(x)
#else
#define _RBT_ASSERT(x)		do {} while (0)
#endif

/*
 * internal use macros
 */
#define _RBT_LOWMASK		((__uintptr_t)3U)
#define _RBT_PTR(elm)		(__typeof(elm))((__uintptr_t)(elm) & ~_RBT_LOWMASK)
#define _RBT_BITS(elm)		(*(__uintptr_t *)&elm)

#define _RBT_LDIR		((__uintptr_t)0U)
#define _RBT_RDIR		((__uintptr_t)1U)
#define _RBT_ODIR(dir)		((dir) ^ 1U)


#ifdef RBT_SMALL

#define _RBT_GET_PARENT(elm, oelm)		do {} while (0)
#define _RBT_SET_PARENT(elm, pelm)		do {} while (0)

#define _RBT_STACK_SIZE(head, sz)	do {	\
*sz = (head)->top;				\
} while (0)

#define _RBT_STACK_PUSH(head, elm)	do {	\
(head)->stack[(head)->top++] = elm;		\
} while (0)

#define _RBT_STACK_DROP(head)		do {	\
(head)->top -= 1;				\
} while (0)

#define _RBT_STACK_POP(head, oelm)	do {	\
if ((head)->top > 0)				\
	oelm = (head)->stack[--(head)->top];	\
} while (0)

#define _RBT_STACK_TOP(head, oelm)	do {	\
if ((head)->top > 0)				\
	oelm = (head)->stack[(head)->top - 1];	\
} while (0)

#define _RBT_STACK_CLEAR(head)		do {	\
(head)->top = 0;				\
_RBT_STACK_PUSH(head, NULL);			\
} while (0)

#define _RBT_STACK_SET(head, i, elm)	do {	\
(head)->stack[i] = elm;				\
} while (0)

#else

#define _RBT_PDIR			((__uintptr_t)2U)

#define _RBT_GET_PARENT(elm, pelm)	do {	\
pelm = _RBT_GET_CHILD(elm, _RBT_PDIR);			\
} while (0)

#define _RBT_SET_PARENT(elm, pelm) do {		\
_RBT_SET_CHILD(elm, _RBT_PDIR, pelm);		\
} while (0)

#define _RBT_STACK_SIZE(head, sz)		do {} while (0)
#define _RBT_STACK_PUSH(head, elm)		do {} while (0)
#define _RBT_STACK_DROP(head)			do {} while (0)
#define _RBT_STACK_POP(head, elm)		do {} while (0)
#define _RBT_STACK_TOP(head, elm)		do {} while (0)
#define _RBT_STACK_CLEAR(head)			do {} while (0)
#define _RBT_STACK_SET(head, i, elm)		do {} while (0)

#endif



/*
 * element macros
 */
#define _RBT_GET_CHILD(elm, dir)				(elm)->child[dir]
#define _RBT_SET_CHILD(elm, dir, celm) do {			\
_RBT_GET_CHILD(elm, dir) = (celm);				\
} while (0)
#define _RBT_REPLACE_CHILD(elm, dir, oelm, nelm) do {		\
_RBT_BITS(_RBT_GET_CHILD(elm, dir)) ^= _RBT_BITS(oelm) ^ _RBT_BITS(nelm);	\
} while (0)
#define _RBT_SWAP_CHILD_OR_ROOT(rbt, elm, oelm, nelm) do {	\
if (elm == NULL)						\
	_RBT_ROOT(rbt) = nelm;					\
else								\
	_RBT_REPLACE_CHILD(elm, (_RBT_LEFT(elm) == (oelm) ? _RBT_LDIR : _RBT_RDIR), oelm, nelm);	\
} while (0)

#define _RBT_GET_RDIFF(elm, dir)				(_RBT_BITS(_RBT_GET_CHILD(elm, dir)) & 1U)
#define _RBT_FLIP_RDIFF(elm, dir) do {				\
_RBT_BITS(_RBT_GET_CHILD(elm, dir)) ^= 1U;			\
} while (0)
#define _RBT_SET_RDIFF0(elm, dir) do {				\
_RBT_BITS(_RBT_GET_CHILD(elm, dir)) &= ~_RBT_LOWMASK;		\
} while (0)
#define _RBT_SET_RDIFF1(elm, dir) do {				\
_RBT_BITS(_RBT_GET_CHILD(elm, dir)) |= 1U;			\
} while (0)


#define _RBT_ROOT(rbt)		(rbt)->root
#define _RBT_EMPTY(rbt)		(_RBT_ROOT(rbt) == NULL)
//#define _RBT_LEFT(elm)		_RBT_PTR(_RBT_GET_CHILD(elm, _RBT_LDIR))
//#define _RBT_RIGHT(elm)		_RBT_PTR(_RBT_GET_CHILD(elm, _RBT_RDIR))

struct rb_entry *
_RBT_LEFT(struct rb_entry *elm)
{
	return _RBT_PTR(_RBT_GET_CHILD(elm, _RBT_LDIR));
}

struct rb_entry *
_RBT_RIGHT(struct rb_entry *elm)
{
	return _RBT_PTR(_RBT_GET_CHILD(elm, _RBT_RDIR));
}


/*
 *      elm              celm
 *      / \              / \
 *     c1  celm   ->    elm gc2
 *         / \          / \
 *      gc1   gc2      c1 gc1
 */
#define _RBT_ROTATE(elm, celm, dir) do {			\
_RBT_SET_CHILD(elm, _RBT_ODIR(dir), _RBT_GET_CHILD(celm, dir));	\
if (_RBT_PTR(_RBT_GET_CHILD(elm, _RBT_ODIR(dir))) != NULL)	\
	_RBT_SET_PARENT(_RBT_PTR(_RBT_GET_CHILD(elm, _RBT_ODIR(dir))), elm);	\
_RBT_SET_CHILD(celm, dir, elm);					\
_RBT_SET_PARENT(elm, celm);					\
} while (0)


/*
 * t_augment should only return true when the update changes the node data,
 * so that updating can be stopped short of the root when it returns false.
 */
static inline void
_rb_augment_walk(struct rb_tree *rbt, struct rb_entry *elm)
{
	while (elm != NULL && (*(rbt->options->t_augment))(rbt, elm)) {
		_RBT_GET_PARENT(elm, elm);
		_RBT_STACK_POP(rbt, elm);
	}
}

static inline void
_rb_augment_try(struct rb_tree *rbt, struct rb_entry *elm)
{
	if (rbt->options->t_augment != NULL)
		(void)(*(rbt->options->t_augment))(rbt, elm);
}

static inline struct rb_entry *
_rb_n2e(const struct rb_type *t, void *node)
{
	unsigned long addr = (unsigned long)node;

	return ((struct rb_entry *)(addr + t->t_offset));
}

static inline void *
_rb_e2n(const struct rb_type *t, struct rb_entry *rbe)
{
	unsigned long addr = (unsigned long)rbe;

	return ((void *)(addr - t->t_offset));
}

static inline int
_rb_cmp(const struct rb_tree *rbt, struct rb_entry *a, struct rb_entry *b)
{
	return ((*(rbt->options->t_compare))(a, b));
}

void
rb_init(struct rb_tree *rbt)
{
	(rbt)->root = NULL;
	_RBT_STACK_CLEAR(rbt);
}

int
rb_empty(struct rb_tree *rbt)
{
	return (_RBT_EMPTY(rbt));
}

/*
 * returns -2 if the subtree is not rank balanced else returns
 * the rank of the node.
 */
static inline int
_rb_rank(struct rb_entry *elm)
{
	int lrank, rrank;
	if (elm == NULL)
		return (-1);
	lrank =  _rb_rank(_RBT_LEFT(elm));
	if (lrank < -2)
		return (-4);
	rrank =  _rb_rank(_RBT_RIGHT(elm));
	if (rrank < -2)
		return (-8);
	lrank += (_RBT_GET_RDIFF(elm, _RBT_LDIR) == 1U) ? 2 : 1;
	rrank += (_RBT_GET_RDIFF(elm, _RBT_RDIR) == 1U) ? 2 : 1;
	if (lrank != rrank)
		return (-2);
	return (lrank);
}

int
rb_rank(struct rb_tree *rbt)
{
	return (_rb_rank(_RBT_ROOT(rbt)));
}

int rb_rank_node(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	return (_rb_rank(elm));
}

int
rb_rank_diff(struct rb_tree *rbt, void *node, int dir)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	return (_RBT_GET_RDIFF(elm, dir));
}

void *
rb_root(struct rb_tree *rbt)
{
	if (_RBT_EMPTY(rbt))
		return (NULL);
	return (_rb_e2n(rbt->options, _RBT_ROOT(rbt)));
}

static inline struct rb_entry *
_rb_minmax(struct rb_tree *rbt, int dir)
{
	struct rb_entry *tmp = _RBT_ROOT(rbt);
	struct rb_entry *parent = NULL;
	while (tmp) {
		parent = tmp;
		tmp = _RBT_PTR(_RBT_GET_CHILD(tmp, dir));
	}
	return (parent);
}

static inline struct rb_entry *
_rb_min(struct rb_tree *rbt)
{
	return _rb_minmax(rbt, _RBT_LDIR);
}

static inline struct rb_entry *
_rb_max(struct rb_tree *rbt)
{
	return _rb_minmax(rbt, _RBT_RDIR);
}

void *
rb_min(struct rb_tree *rbt)
{
        return _rb_e2n(rbt->options, _rb_min(rbt));
}

void *
rb_max(struct rb_tree *rbt)
{
        return _rb_e2n(rbt->options, _rb_max(rbt));
}

static inline struct rb_entry *
_rb_find(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *tmp = _RBT_ROOT(rbt);
	int comp;
	while (tmp) {
		comp = _rb_cmp(rbt, elm, tmp);
		if (comp < 0)
			tmp = _RBT_LEFT(tmp);
		else if (comp > 0)
			tmp = _RBT_RIGHT(tmp);
		else
			return (tmp);
	}
	return (NULL);
}

void *
rb_find(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *res = _rb_find(rbt, elm);
	if (res == NULL)
		return (NULL);
	return (_rb_e2n(rbt->options, res));
}

static inline struct rb_entry *
_rb_nfind(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *tmp = _RBT_ROOT(rbt);
	struct rb_entry *res = NULL;
	int comp = 0;
	while (tmp) {
		comp = _rb_cmp(rbt, elm, tmp);
		if (comp < 0) {
			res = tmp;
			tmp = _RBT_LEFT(tmp);
		}
		else if (comp > 0)
			tmp = _RBT_RIGHT(tmp);
		else
			return (tmp);
	}
	return (res);
}

void *
rb_nfind(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *res = _rb_nfind(rbt, elm);
	if (res == NULL)
		return (NULL);
	return (_rb_e2n(rbt->options, res));
}

static inline struct rb_entry *
_rb_pfind(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *tmp = _RBT_ROOT(rbt);
	struct rb_entry *res = NULL;
	int comp = 0;
	while (tmp) {
		comp = _rb_cmp(rbt, elm, tmp);
		if (comp > 0) {
			res = tmp;
			tmp = _RBT_RIGHT(tmp);
		}
		else if (comp < 0)
			tmp = _RBT_LEFT(tmp);
		else
			return (tmp);
	}
	return (res);
}

void *
rb_pfind(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *res = _rb_pfind(rbt, elm);
	if (res == NULL)
		return (NULL);
	return (_rb_e2n(rbt->options, res));
}

#ifdef RBT_SMALL
static inline struct rb_entry *
_rb_findc(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *tmp = _RBT_ROOT(rbt);
	int comp;
	_RBT_STACK_CLEAR(rbt);
	while (tmp) {
		_RBT_STACK_PUSH(rbt, tmp);
		comp = _rb_cmp(rbt, elm, tmp);
		if (comp < 0)
			tmp = _RBT_LEFT(tmp);
		else if (comp > 0)
			tmp = _RBT_RIGHT(tmp);
		else
			return (tmp);
	}
	return (NULL);
}

#else
#define _rb_findc(rbt, elm)	(elm)
#endif


/*
 * When doing a balancing of the tree, lets check when we are looking
 * at the edge 'elm' to its 'parent'.
 * We assume that 'elm' has already been promoted.
 * Now there are two possibilities:
 * 1) if 'elm' has rank difference 1 with 'parent', or it is the root
 *    node, we are done.
 * 2) if 'elm' has rank difference 0 with 'parent', we have a few cases:
 *
 * 2.1) the sibling of 'elm' has rank difference 1 with 'parent' then
 *      we promote the 'parent'. And continue recursively from parent.
 *
 *                gpar                          gpar
 *                 /                             /(0/1)
 *               1/2                          parent
 *               /                            1/  \
 *   elm --0-- parent           -->         elm    2
 *    /\          \                          /\     \
 *    --           1                         --    sibling
 *                  \                                /\
 *                   sibling                         --
 *                   /\
 *                   --
 *
 * 2.2) the sibling of 'elm' has rank difference 2 with 'parent',
 *      then we need to do rotations based on the child of 'elm'
 *      that has rank difference 1 with 'elm'.
 *      there will always be one such child as 'elm' had to be promoted.
 *
 * 2.2a) the rdiff 1 child of 'elm', 'c', is in the same direction
 *       as 'elm' is wrt to 'parent'. We demote the parent and do
 *       a single rotation in the opposite direction and we are done.
 *
 *                       gpar                         gpar
 *                        /                            /
 *                      1/2                          1/2
 *                      /                            /
 *          elm --0-- parent           -->         elm
 *          / \          \                         / \
 *         1   2          2                       1   1
 *        /     \          \                     /     \
 *       c       d         sibling              c      parent
 *      /\       /\          /\                /\        / \
 *      --       --          --                --       1   1
 *                                                     /     \
 *                                                    d    sibling
 *                                                   /\      /\
 *                                                   --      --
 *
 *  2.2b) the rdiff 1 child of 'elm', 'c', is in the opposite
 *        direction as 'elm' is wrt to 'parent'. We do a double
 *        rotation (with rank changes) and we are done.
 *
 *                       gpar                         gpar
 *                        /                            /
 *                      1/2                          1/2
 *                      /                            /
 *          elm --0-- parent           -->          c
 *          / \          \                        1/ \1
 *         2   1          2                     elm   parent
 *        /     \          \                  1/  \      /  \1
 *       d       c         sibling            d    c1  c2  sibling
 *      /\      / \          /\              /\    /\  /\    /\
 *      --     c1 c2         --              --    --  --    --
 */
static inline struct rb_entry *
_rb_insert_balance(struct rb_tree *rbt, struct rb_entry *parent,
    struct rb_entry *elm)
{
	struct rb_entry *child, *gpar;
	__uintptr_t elmdir, sibdir;

	child = NULL;
	gpar = NULL;
	do {
		/* elm has not been promoted yet */
		elmdir = _RBT_LEFT(parent) == elm ? _RBT_LDIR : _RBT_RDIR;
		if (_RBT_GET_RDIFF(parent, elmdir)) {
			/* case (1) */
			_RBT_FLIP_RDIFF(parent, elmdir);
			_RBT_STACK_PUSH(rbt, parent);
			return (elm);
		}
		_RBT_STACK_POP(rbt, gpar);
		_RBT_GET_PARENT(parent, gpar);
		/* case (2) */
		sibdir = _RBT_ODIR(elmdir);
		_RBT_FLIP_RDIFF(parent, sibdir);
		if (_RBT_GET_RDIFF(parent, sibdir)) {
			/* case (2.1) */
			_rb_augment_try(rbt, elm);
			elm = parent;
			continue;
		}
		_RBT_SET_RDIFF0(elm, elmdir);
		/* case (2.2) */
		if (_RBT_GET_RDIFF(elm, sibdir) == 0) {
			/* case (2.2b) */
			child = _RBT_PTR(_RBT_GET_CHILD(elm, sibdir));
			_RBT_ROTATE(elm, child, elmdir);
		} else {
			/* case (2.2a) */
			child = elm;
			_RBT_FLIP_RDIFF(elm, sibdir);
		}
		_RBT_ROTATE(parent, child, sibdir);
		_RBT_SET_PARENT(child, gpar);
		_RBT_SWAP_CHILD_OR_ROOT(rbt, gpar, parent, child);
		if (rbt->options->t_augment != NULL) {
			(void)(*(rbt->options->t_augment))(rbt, parent);
			if (elm != child)
				(void)(*(rbt->options->t_augment))(rbt, elm);
		}
		_RBT_STACK_PUSH(rbt, gpar);
		return (child);
	} while ((parent = gpar) != NULL);
	_RBT_STACK_PUSH(rbt, NULL);
	return (elm);
}


static inline struct rb_entry *
_rb_insert_finish(struct rb_tree *rbt, struct rb_entry *parent, 
    __uintptr_t insdir, struct rb_entry *elm)
{
	struct rb_entry *tmp = elm;
	_RBT_SET_PARENT(elm, parent);
	if (_RBT_GET_CHILD(parent, insdir))
		_RBT_SET_CHILD(parent, insdir, elm);
	else {
		_RBT_SET_CHILD(parent, insdir, elm);
		tmp = _rb_insert_balance(rbt, parent, elm);
		_RBT_STACK_POP(rbt, parent);
		_RBT_GET_PARENT(tmp, parent);
	}
	if (rbt->options->t_augment != NULL) {
		(void)(*(rbt->options->t_augment))(rbt, tmp);
		_rb_augment_walk(rbt, parent);
	}
	return (NULL);
}


static inline struct rb_entry *
_rb_insert(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *parent, *tmp;
	int comp;
	__uintptr_t insdir;

	_RBT_STACK_CLEAR(rbt);
	_RBT_SET_CHILD(elm, _RBT_LDIR, NULL);
	_RBT_SET_CHILD(elm, _RBT_RDIR, NULL);
	tmp = _RBT_ROOT(rbt);
	if (tmp == NULL) {
		_RBT_ROOT(rbt) = elm;
		_RBT_SET_PARENT(elm, NULL);
		return (NULL);
	}
	while (tmp) {
		parent = tmp;
		comp = _rb_cmp(rbt, elm, tmp);
		if (comp < 0) {
			tmp = _RBT_LEFT(tmp);
			insdir = _RBT_LDIR;
		}
		else if (comp > 0) {
			tmp = _RBT_RIGHT(tmp);
			insdir = _RBT_RDIR;
		}
		else
			return (parent);
		_RBT_STACK_PUSH(rbt, parent);
	}
	_RBT_STACK_POP(rbt, parent);
	return _rb_insert_finish(rbt, parent, insdir, elm);
}

void *
rb_insert(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *res = _rb_insert(rbt, elm);
	if (res == NULL)
		return (NULL);
	return (_rb_e2n(rbt->options, res));
}

#ifndef RBT_SMALL
static inline struct rb_entry *
_rb_insert_next(struct rb_tree *rbt, struct rb_entry *elm,
    struct rb_entry *next)
{
	struct rb_entry *tmp;
	__uintptr_t insdir = _RBT_RDIR;
	_RBT_SET_CHILD(next, _RBT_LDIR, NULL);
	_RBT_SET_CHILD(next, _RBT_RDIR, NULL);

	tmp = _RBT_RIGHT(elm);
	while (tmp) {
		elm = tmp;
		tmp = _RBT_LEFT(tmp);
		insdir = _RBT_LDIR;
	}
	return _rb_insert_finish(rbt, elm, insdir, next);
}

static inline struct rb_entry *
_rb_insert_prev(struct rb_tree *rbt, struct rb_entry *elm,
    struct rb_entry *prev)
{
	struct rb_entry *tmp;
	__uintptr_t insdir = _RBT_LDIR;
	_RBT_SET_CHILD(prev, _RBT_LDIR, NULL);
	_RBT_SET_CHILD(prev, _RBT_RDIR, NULL);

	tmp = _RBT_RIGHT(elm);
	while (tmp) {
		elm = tmp;
		tmp = _RBT_LEFT(tmp);
		insdir = _RBT_RDIR;
	}
	return _rb_insert_finish(rbt, elm, insdir, prev);
}
#endif

/*
 * When doing a balancing of the tree after a removal, lets check
 * when we are looking at the edge 'elm' to its 'parent'.
 * For now, assume that 'elm' has already been demoted.
 * Now there are two possibilities:
 * 1) if 'elm' has rank difference 2 with 'parent', or it is
 *    the root node, we are done.
 * 2) if 'elm' has rank difference 3 with 'parent', we have a
 *    few cases:
 *
 * 2.1) the sibling of 'elm' has rank difference 2 with 'parent'
 *      then we demote the 'parent'. And continue recursively from
 *      parent.
 *
 *                gpar                          gpar
 *                 /                             /
 *               1/2                           2/3
 *               /                             /
 *          parent           -->              /
 *          /    \                         parent
 *         3      2                       /      \ 1
 *        /        \                     2        \
 *       /         sibling              /        sibling
 *     elm            /\              elm           /\
 *      /\            --               /\           --
 *      --                             --
 * 
 * 2.2) the sibling of 'elm' has rank difference 1 with 'parent', then
 *      we need to do rotations, based on the children of the
 *      sibling of elm.
 *
 * 2.2a) Both children of sibling have rdiff 2. Then we demote both
 *       parent and sibling and continue recursively from parent.
 *
 *                gpar                          gpar
 *                 /                             /
 *               1/2                           2/3
 *               /                             /
 *          parent           -->           parent
 *          /    \                         /    \
 *         3      1                       2      1
 *        /        \                     /        \
 *      elm         sibling            elm         sibling
 *      /\          2/   \2             /\         1/   \1
 *      --          c     d             --         c     d
 *
 *
 *  2.2b) the rdiff 1 child of 'sibling', 'c', is in the same
 *        direction as 'sibling' is wrt to 'parent'. We do a single
 *        rotation (with rank changes) and we are done.
 *
 *            gpar                    gpar                        gpar
 *             /                       /                           /
 *           1/2                     1/2      if                 1/2
 *           /                       /    parent->c == 2         /
 *      parent        -->       sibling      -->            sibling 
 *      /    \                  1/    \                     2/    \
 *     3      1               parent   2                 parent    2
 *    /        \              2/   \    \                1/   \1    \
 *  elm         sibling      elm    c    d              elm    c     d
 *  /\           /   \1      /\                          /\
 *  --          c     d      --                          --
 *
 * 2.2c) the rdiff 1 child of 'sibling', 'c', is in the opposite
 *      direction as 'sibling' is wrt to 'parent'. We do a double
 *      rotation (with rank changes) and we are done.
 *
 *                gpar                          gpar
 *                 /                             /
 *               1/2                           1/2
 *               /                             /
 *          parent           -->              c
 *          /    \                          2/ \2
 *         3      1                     parent  sibling
 *        /        \                   1/   \    /   \1
 *      elm         sibling           elm   c1  c2    d
 *      /\          1/   \2           /\
 *      --          c     d           --
 *                 / \
 *                c1  c2
 *
 */
static inline struct rb_entry *
_rb_remove_balance(struct rb_tree *rbt,
    struct rb_entry *parent, struct rb_entry *elm)
{
	struct rb_entry *gpar, *sibling, *tmp1 = NULL, *tmp2 = NULL;
	__uintptr_t sibdir, ssdiff, sodiff;
	volatile __uintptr_t elmdir;
	int extend;

	_RBT_ASSERT(parent != NULL);
	gpar = NULL;
	sibling = NULL;
	if (_RBT_RIGHT(parent) == NULL && _RBT_LEFT(parent) == NULL) {
		fprintf(stderr, "case XXX - parent = %p\n", parent);
		_RBT_SET_CHILD(parent, _RBT_LDIR, NULL);
		_RBT_SET_CHILD(parent, _RBT_RDIR, NULL);
		elm = parent;
		_rb_augment_try(rbt, elm);
		_RBT_STACK_POP(rbt, parent);
		_RBT_GET_PARENT(parent, parent);
		if (parent == NULL) {
			return (NULL);
		}
	}
	do {
		assert(parent != NULL);
		_RBT_STACK_POP(rbt, gpar);
		_RBT_GET_PARENT(parent, gpar);
		// fprintf(stderr, "parent = %p\n", parent);
		// fprintf(stderr, "elmdir = %llu\n", elmdir);
		// fprintf(stderr, "elm = %p\n", (void *)elm);
		// fprintf(stderr, "left child = %p\n", _RBT_LEFT(parent));
		// fprintf(stderr, "right child = %p\n", _RBT_RIGHT(parent));
		tmp1 = _RBT_LEFT(parent);
		tmp2 = _RBT_RIGHT(parent);
		if (tmp1 == elm)
			elmdir = _RBT_LDIR;
		else if (tmp2 == elm)
			elmdir = _RBT_RDIR;
		else
			assert(0);
		tmp1 = _RBT_LEFT(parent);
		tmp2 = _RBT_RIGHT(parent);
		if (tmp1 == elm)
			elmdir = _RBT_LDIR;
		else if (tmp2 == elm)
			elmdir = _RBT_RDIR;
		else
			assert(0);
		//_RBT_GET_RDIFF(parent, elmdir);
		elmdir = (elm == (_RBT_LEFT(parent))) ? _RBT_LDIR : _RBT_RDIR;
		if (_RBT_GET_RDIFF(parent, elmdir) == 0) {
			/* case (1) */
			fprintf(stderr, "case 1\n");
			fprintf(stderr, "parent = %p\n", parent);
			fprintf(stderr, "elm = %p\n", elm);
			fprintf(stderr, "elmdir = %ul\n", elmdir);
			fprintf(stderr, "left child = %p\n", _RBT_LEFT(parent));
			fprintf(stderr, "right child = %p\n", _RBT_RIGHT(parent));
			if (_RBT_LEFT(parent) == elm) {
				fprintf(stderr, "it is left child\n");
			}
			_RBT_FLIP_RDIFF(parent, elmdir);
			fprintf(stderr, "rdiff = %d\n", _RBT_GET_RDIFF(parent, elmdir));
			_RBT_STACK_PUSH(rbt, gpar);
			return (parent);
		}
		/* case 2 */
		sibdir = _RBT_ODIR(elmdir);
		if (_RBT_GET_RDIFF(parent, sibdir)) {
			/* case 2.1 */
			fprintf(stderr, "case 2.1\n");
			_RBT_FLIP_RDIFF(parent, sibdir);
			_rb_augment_try(rbt, parent);
			continue;
		}
		/* case 2.2 */
		sibling = _RBT_PTR(_RBT_GET_CHILD(parent, sibdir));
		_RBT_ASSERT(sibling != NULL);
		ssdiff = _RBT_GET_RDIFF(sibling, elmdir);
		sodiff = _RBT_GET_RDIFF(sibling, sibdir);
		if (ssdiff && sodiff) {
			/* case 2.2a */
			fprintf(stderr, "case 2.2a\n");
			_RBT_FLIP_RDIFF(sibling, elmdir);
			_RBT_FLIP_RDIFF(sibling, sibdir);
			_rb_augment_try(rbt, parent);
			continue;
		}
		extend = 0;
		if (sodiff) {
			/* case 2.2c */
			fprintf(stderr, "case 2.2c\n");
			_RBT_FLIP_RDIFF(sibling, sibdir);
			_RBT_FLIP_RDIFF(parent, elmdir);
			elm = _RBT_PTR(_RBT_GET_CHILD(sibling, elmdir));
			_RBT_ROTATE(sibling, elm, sibdir);
			_RBT_SET_RDIFF1(elm, sibdir);
			extend = 1;
		} else {
			/* case 2.2b */
			fprintf(stderr, "case 2.2b\n");
			_RBT_FLIP_RDIFF(sibling, sibdir);
			if (ssdiff) {
				_RBT_FLIP_RDIFF(sibling, elmdir);
				_RBT_FLIP_RDIFF(parent, elmdir);
				extend = 1;
			}
			_RBT_FLIP_RDIFF(parent, sibdir);
			elm = sibling;
		}
		_RBT_ROTATE(parent, elm, elmdir);
		_RBT_SET_PARENT(elm, gpar);
		_RBT_SWAP_CHILD_OR_ROOT(rbt, gpar, parent, elm);
		if (extend) {
			_RBT_SET_RDIFF1(elm, elmdir);
		}
		if (rbt->options->t_augment != NULL) {
			(void)(*(rbt->options->t_augment))(rbt, parent);
			if (elm != sibling)
				(void)(*(rbt->options->t_augment))(rbt, sibling);
		}
		_RBT_STACK_PUSH(rbt, gpar);
		return (elm);
	} while ((elm = parent, (parent = gpar) != NULL));
	_RBT_STACK_PUSH(rbt, NULL);
	return (elm);
}

static inline struct rb_entry *
_rb_remove_start(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *parent, *opar, *child, *rmin, *cptr;
	size_t sz;

	parent = NULL;
	opar = NULL;
	_RBT_STACK_TOP(rbt, opar);
	_RBT_GET_PARENT(elm, opar);

	/* first find the element to swap with oelm */
	child = _RBT_GET_CHILD(elm, _RBT_LDIR);
	cptr = _RBT_PTR(child);
	rmin = _RBT_RIGHT(elm);
	if (rmin == NULL || cptr == NULL) {
		rmin = child = (rmin == NULL ? cptr : rmin);
		parent = opar;	
		_RBT_STACK_DROP(rbt);
	}
	else {
		_RBT_STACK_PUSH(rbt, elm);
		_RBT_STACK_SIZE(rbt, &sz);
		parent = rmin;
		while (_RBT_LEFT(rmin)) {
			_RBT_STACK_PUSH(rbt, rmin);
			rmin = _RBT_LEFT(rmin);
		}
		_RBT_SET_CHILD(rmin, _RBT_LDIR, child);
		_RBT_SET_PARENT(cptr, rmin);
		child = _RBT_GET_CHILD(rmin, _RBT_RDIR);
		if (parent != rmin) {
			_RBT_SET_PARENT(parent, rmin);
			_RBT_SET_CHILD(rmin, _RBT_RDIR, _RBT_GET_CHILD(elm, _RBT_RDIR));
			_RBT_GET_PARENT(rmin, parent);
			_RBT_STACK_POP(rbt, parent);
			_RBT_REPLACE_CHILD(parent, _RBT_LDIR, child, rmin);
			_RBT_STACK_SET(rbt, sz - 1, rmin);
		} else {
			_RBT_STACK_SET(rbt, sz - 1, NULL);
			_RBT_STACK_DROP(rbt);
			if (_RBT_GET_RDIFF(elm, _RBT_RDIR))
				_RBT_SET_RDIFF1(rmin, _RBT_RDIR);
		}
		_RBT_SET_PARENT(rmin, opar);
	}
	_RBT_SWAP_CHILD_OR_ROOT(rbt, opar, elm, rmin);
	if (child != NULL) {
		_RBT_SET_PARENT(child, parent);
	}
	if (parent != NULL) {
		parent = _rb_remove_balance(rbt, parent, child);
		if ((rbt->options->t_augment) != NULL)
			_rb_augment_walk(rbt, parent);
	}
	return (elm);
}

static inline struct rb_entry *
_rb_remove(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *telm = elm;

	telm = _rb_findc(rbt, elm);
	if (telm == NULL)
		return (NULL);
	_RBT_STACK_POP(rbt, telm);
	return _rb_remove_start(rbt, telm);
}

void *
rb_remove(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	elm = _rb_remove(rbt, elm);
	return (elm == NULL ? NULL : _rb_e2n(rbt->options, elm));
}

static inline struct rb_entry *
_rb_next(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *parent = NULL;

	elm = _rb_findc(rbt, elm);
	if (elm == NULL)
		return NULL;

	if (_RBT_RIGHT(elm)) {
		elm = _RBT_RIGHT(elm);
		while (_RBT_LEFT(elm))
			elm = _RBT_LEFT(elm);
	} else {
		_RBT_GET_PARENT(elm, parent);
		_RBT_STACK_POP(rbt, parent);
		while (parent && elm == _RBT_RIGHT(parent)) {
			elm = parent;
			_RBT_GET_PARENT(parent, parent);
			_RBT_STACK_POP(rbt, parent);
		}
		elm = parent;
	}
	return (elm);
}

void *
rb_next(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	elm = _rb_next(rbt, elm);
	return (elm == NULL ? NULL : _rb_e2n(rbt->options, elm));
}

static inline struct rb_entry *
_rb_prev(struct rb_tree *rbt, struct rb_entry *elm)
{
	struct rb_entry *parent = NULL;

	elm = _rb_findc(rbt, elm);
	if (elm == NULL)
		return NULL;

	if (_RBT_LEFT(elm)) {
		elm = _RBT_LEFT(elm);
		while (_RBT_RIGHT(elm))
			elm = _RBT_RIGHT(elm);
	} else {
		_RBT_GET_PARENT(elm, parent);
		_RBT_STACK_POP(rbt, parent);
		while (parent && elm == _RBT_LEFT(parent)) {
			elm = parent;
			_RBT_GET_PARENT(parent, parent);
			_RBT_STACK_POP(rbt, parent);
		}
		elm = parent;
	}
	return (elm);
}

void *
rb_prev(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	elm = _rb_prev(rbt, elm);
	return (elm == NULL ? NULL : _rb_e2n(rbt->options, elm));
}


void *
rb_left(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	elm = _RBT_LEFT(elm);
	return (elm == NULL ? NULL : _rb_e2n(rbt->options, elm));
}

void *
rb_right(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	elm = _RBT_RIGHT(elm);
	return (elm == NULL ? NULL : _rb_e2n(rbt->options, elm));
}

void *
rb_parent(struct rb_tree *rbt, void *node)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *tmp = NULL;
	_RBT_GET_PARENT(elm, tmp);
	if (tmp != NULL)
		return _rb_e2n(rbt->options, tmp);
	return NULL;
}

static inline void
_rb_set_child(struct rb_tree *rbt, void *node, __uintptr_t dir, void *child)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *c = _rb_n2e(rbt->options, child);

	_RBT_SET_CHILD(elm, dir, c);
	_RBT_SET_PARENT(c, elm);
}

void
rb_set_left(struct rb_tree *rbt, void *node, void *left)
{
	_rb_set_child(rbt, node, _RBT_LDIR, left);
}

void
rb_set_right(struct rb_tree *rbt, void *node, void *right)
{
	_rb_set_child(rbt, node, _RBT_RDIR, right);
}

void
rb_set_parent(struct rb_tree *rbt, void *node, void *parent)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);
	struct rb_entry *p = _rb_n2e(rbt->options, parent);

	_RBT_SET_PARENT(elm, p);
}

void
rb_poison(struct rb_tree *rbt, void *node, unsigned long poison)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);

	_RBT_SET_PARENT(elm, (struct rb_entry*)poison);
	_RBT_SET_CHILD(elm, _RBT_LDIR, (struct rb_entry*)poison);
	_RBT_SET_CHILD(elm, _RBT_RDIR, (struct rb_entry*)poison);
}

int
rb_check(const struct rb_tree *rbt, void *node, unsigned long poison)
{
	struct rb_entry *elm = _rb_n2e(rbt->options, node);

#ifndef RBT_SMALL
	struct rb_entry *tmp;
	_RBT_GET_PARENT(elm, tmp);
	if (tmp != (struct rb_entry *)poison)
		return (0);
#endif
	return (_RBT_LEFT(elm) == (struct rb_entry *)poison &&
		_RBT_RIGHT(elm) == (struct rb_entry *)poison);
}
