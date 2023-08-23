/*
 * Copyright 2023 Aisha Tammy <aisha@openbsd.org>
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

#ifndef _SYS_RBTREE_H_
#define _SYS_RBTREE_H_

#include <stddef.h>
#include <stdint.h>

/*
 * This code is an implementation of the rank balanced tree with weak AVL conditions.
 * The following paper describes the rank balanced trees in detail:
 *   Haeupler, Sen and Tarjan, "Rank Balanced Trees",
 *   ACM Transactions on Algorithms Volume 11 Issue 4 June 2015
 *   Article No.: 30pp 1–26 https://doi.org/10.1145/2689412
 *
 * A rank-balanced tree is a binary search tree with an integer rank-difference
 * between itself and its parent as an attribute of the node.
 * The sum of the rank-differences on any path from a node down to null is
 * the same, and defines the rank of that node.
 * Leaves have rank 0 and null nodes have rank -1.
 *
 * Weak AVL trees have the nice property that any insertion/deletion can cause
 * at most two rotations, which is an improvement over the standard AVL and Red-Black trees.
 * As a comparison matrix:
 *   Tree type                      Weak AVL       AVL              Red-Black
 *   worst case height              2Log(N)        1.44Log(N)       2Log(N)
 *   height with only insertions    1.44Log(N)     1.44Log(N)       2Log(N)
 *   max rotations after insert     2              O(Log(N))        2
 *   max rotations after delete     2              2                3
 * 
 * For each node we store the left and right pointers (and parent pointer depending on RB2_SMALL).
 * We assume that a pointer to a struct is aligned to multiple of 4 bytes, which leaves
 * the last two bits free for our use.
 * The last bit of each child is used to store the rank difference between the node and its child.
 * The convention used here is the rank difference = 1 + last bit.
 */


#ifndef __uintptr_t
#define __uintptr_t uintptr_t
#endif

/*
 * debug macros
 */
#ifndef RB2_DIAGNOSTIC
#define _RB2_ASSERT(x)		do {} while (0)
#define DEBUGF(fmt, ...)	do {} while (0)
#else
#include <assert.h>
#define _RB2_ASSERT(x)		assert(x)
#define DEBUGF(fmt, ...)	fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#endif

/* 
 * internal use macros
 */
#define _RB2_LOWMASK					((__uintptr_t)3U)
#define _RB2_PTR(elm)					(__typeof(elm))((__uintptr_t)(elm) & ~_RB2_LOWMASK)
/* this is used for converting a struct type * to a __uintptr_t and can also be used as an lvalue */
#define _RB2_BITS(elm)					(*(__uintptr_t *)&elm)

#define _RB2_LDIR					((__uintptr_t)0U)
#define _RB2_RDIR					((__uintptr_t)1U)
#define _RB2_ODIR(dir)					((dir) ^ 1U)

#ifndef RB2_MAX_HEIGHT
#define RB2_MAX_HEIGHT					127
#endif

#ifdef RB2_SMALL

#define RB2_ENTRY(type)					\
struct {						\
	/* left, right */				\
	struct type	*child[2];			\
}

#define RB2_HEAD(name, type)				\
struct name {						\
	struct type	*root;				\
	struct type	*stack[RB2_MAX_HEIGHT];		\
	size_t		 top;				\
}

#define _RB2_GET_PARENT(elm, oelm, field)		do {} while (0)
#define _RB2_SET_PARENT(elm, pelm, field)		do {} while (0)

#define _RB2_STACK_SIZE(head, sz)	do {	\
*sz = (head)->top;				\
} while (0)

#define _RB2_STACK_PUSH(head, elm)	do {	\
(head)->stack[head->top++] = elm;		\
} while (0)

#define _RB2_STACK_POP(head, oelm)	do {	\
if ((head)->top > 0)				\
	oelm = (head)->stack[--(head)->top];	\
} while (0)

#define _RB2_STACK_TOP(head, oelm)	do {	\
if ((head)->top > 0)				\
	oelm = (head)->stack[(head)->top - 1];	\
} while (0)

#define _RB2_STACK_CLEAR(head)		do {	\
(head)->top = 0;				\
} while (0)

#define _RB2_STACK_SET(head, i, elm) do {	\
(head)->stack[i] = elm;				\
} while (0)

#define RB2_INIT(head)			do {	\
(head)->root = NULL;				\
_RB2_STACK_CLEAR(head);				\
} while (0)

#else

#define RB2_ENTRY(type)					\
struct {						\
	/* left, right, parent */			\
	struct type *child[3];				\
}

#define RB2_HEAD(name, type)				\
struct name {						\
	struct type *root;				\
}

#define _RB2_PDIR					2

#define _RB2_GET_PARENT(elm, field)			_RB2_CHILD(elm, _RB2_PDIR, field)

#define _RB2_SET_PARENT(elm, pelm, field) do {		\
	_RB2_SPTR(elm, _RB2_PDIR, field) = (pelm);	\
} while (0)

#define _RB2_STACK_PUSH(name, head, elm)		do {} while (0)
#define _RB2_STACK_POP(name, head)			do {} while (0)
#define _RB2_STACK_TOP(name, head)			do {} while (0)

#endif

/*
 * element macros
 */
#define _RB2_GET_CHILD(elm, dir, field)			(elm)->field.child[dir]
#define _RB2_SET_CHILD(elm, dir, celm, field)		do {	\
_RB2_GET_CHILD(elm, dir, field) = (celm);			\
} while (0)
#define _RB2_REPLACE_CHILD(elm, dir, oelm, nelm, field)	do {	\
_RB2_BITS(_RB2_GET_CHILD(elm, dir, field)) ^= (__uintptr_t)_RB2_BITS(oelm) ^ _RB2_BITS(nelm);	\
} while (0)
#define _RB2_SWAP_CHILD_OR_ROOT(head, elm, oelm, nelm, field)	do {	\
if (elm == NULL)							\
	RB2_ROOT(head) = nelm;						\
else									\
	_RB2_REPLACE_CHILD(elm, (RB2_LEFT(elm, field) == (oelm) ? _RB2_LDIR : _RB2_RDIR), oelm, nelm, field);	\
} while (0)

#define _RB2_GET_RDIFF(elm, dir, field)			(_RB2_BITS(_RB2_GET_CHILD(elm, dir, field)) & 1U)
#define _RB2_FLIP_RDIFF(elm, dir, field)		do {	\
_RB2_BITS(_RB2_GET_CHILD(elm, dir, field)) ^= 1U;		\
} while (0)
#define _RB2_SET_RDIFF(elm, dir, rdiff, field)		do {	\
_RB2_ASSERT(rdiff == 0 || rdiff == 1);				\
_RB2_BITS(_RB2_GET_CHILD(elm, dir, field)) = (_RB2_BITS(_RB2_GET_CHILD(elm, dir, field)) & ~_RB2_LOWMASK) | (rdiff);	\
} while (0)


#define RB2_ROOT(head)					(head)->root
#define RB2_LEFT(elm, field)				_RB2_PTR(_RB2_GET_CHILD(elm, _RB2_LDIR, field))
#define RB2_RIGHT(elm, field)				_RB2_PTR(_RB2_GET_CHILD(elm, _RB2_RDIR, field))

/*
 * @param elm: the node to be rotated
 * @param celm: child element
 * @param dir: the direction of rotation
 * 
 * 
 *      elm            celm
 *      / \            / \
 *     c1  celm       elm gc2
 *         / \        / \
 *      gc1   gc2    c1 gc1
 */
#define _RB2_ROTATE(elm, celm, dir, field) do {						\
_RB2_SET_CHILD(elm, _RB2_ODIR(dir), _RB2_GET_CHILD(celm, dir, field), field);		\
if (_RB2_PTR(_RB2_GET_CHILD(elm, _RB2_ODIR(dir), field)) != NULL)			\
	_RB2_SET_PARENT(_RB2_PTR(_RB2_GET_CHILD(elm, _RB2_ODIR(dir), field)), elm, field);	\
_RB2_SET_CHILD(celm, dir, elm, field);							\
_RB2_SET_PARENT(elm, celm, field);							\
} while (0)


/* returns -2 if the subtree is not rank balanced else returns the rank of the node */
#define _RB2_GENERATE_RANK(name, type, field, cmp, attr)				\
attr int										\
name##_RB2_RANK(const struct type *elm)							\
{											\
	int lrank, rrank;								\
	if (elm == NULL)								\
		return (-1);								\
	lrank =  name##_RB2_RANK(RB2_LEFT(elm, field));					\
	if (lrank == -2)								\
		return (-2);								\
	rrank =  name##_RB2_RANK(RB2_RIGHT(elm, field));				\
	if (rrank == -2)								\
		return (-2);								\
	lrank += (_RB2_GET_RDIFF(elm, _RB2_LDIR, field) == 1) ? 2 : 1;			\
	rrank += (_RB2_GET_RDIFF(elm, _RB2_RDIR, field) == 1) ? 2 : 1;			\
	if (lrank != rrank)								\
		return (-2);								\
	return (lrank);									\
}


/* When doing a balancing of the tree, lets check when we are looking at the edge
 * 'elm' to its 'parent'. We assume that 'elm' has already been promoted.
 * Now there are two possibilities:
 * 1) if 'elm' has rank difference 1 with 'parent', or it is the root node, we are done
 * 2) if 'elm' has rank difference 0 with 'parent', we have a few cases
 *
 * 2.1) the sibling of 'elm' has rank difference 1 with 'parent' then we promote the
 *       'parent'. And continue recursively from parent.
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
 * 2.2) the sibling of 'elm' has rank difference 2 with 'parent', then we need to do
 *       rotations, based on the child of 'elm' that has rank difference 1 with 'elm'
 *       (there will always be one such child as 'elm' had to be promoted due to it)
 *
 * 2.2a) the rdiff 1 child of 'elm', 'c', is in the same direction as 'elm' is wrt
 *       to 'parent'. We demote the parent and do a single rotation in the opposite
 *       direction and we are done.
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
 *  2.2b) the rdiff 1 child of 'elm', 'c', is in the opposite direction as 'elm' is wrt
 *       to 'parent'. We do a double rotation (with rank changes) and we are done.
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
#define _RB2_GENERATE_INSERT(name, type, field, cmp, attr)				\
											\
attr struct type *									\
name##_RB2_INSERT_BALANCE(struct name *head, struct type *parent, struct type *elm)	\
{											\
	struct type *child, *gpar;							\
	__uintptr_t elmdir, sibdir;							\
											\
	child = NULL;									\
	gpar = NULL;									\
	do {										\
		/* elm has not been promoted yet */					\
		_RB2_ASSERT(parent != NULL);						\
		_RB2_ASSERT(elm != NULL);						\
		elmdir = RB2_LEFT(parent, field) == elm ? _RB2_LDIR : _RB2_RDIR;	\
		if (_RB2_GET_RDIFF(parent, elmdir, field)) {				\
			/* case (1) */							\
			_RB2_FLIP_RDIFF(parent, elmdir, field);				\
			return (NULL);							\
		}									\
		_RB2_STACK_POP(head, gpar);						\
		_RB2_GET_PARENT(parent, gpar, field);					\
		/* case (2) */								\
		sibdir = _RB2_ODIR(elmdir);						\
		_RB2_FLIP_RDIFF(parent, sibdir, field);					\
		if (_RB2_GET_RDIFF(parent, sibdir, field)) {				\
			/* case (2.1) */						\
			elm = parent;							\
			continue;							\
		}									\
		_RB2_SET_RDIFF(elm, elmdir, 0, field);					\
		/* case (2.2) */							\
		if (_RB2_GET_RDIFF(elm, sibdir, field) == 0) {				\
			/* case (2.2b) */						\
			child = _RB2_PTR(_RB2_GET_CHILD(elm, sibdir, field));		\
			_RB2_ROTATE(elm, child, elmdir, field);				\
		} else {								\
			/* case (2.2a) */						\
			child = elm;							\
			_RB2_FLIP_RDIFF(elm, sibdir, field);				\
		}									\
		_RB2_ROTATE(parent, child, sibdir, field);				\
		_RB2_SET_PARENT(child, gpar, field);					\
		_RB2_SET_CHILD(child, sibdir, parent, field);				\
		if (gpar == NULL) {							\
			RB2_ROOT(head) = child;						\
		} else {								\
			elmdir = RB2_LEFT(gpar, field) == parent ? _RB2_LDIR : _RB2_RDIR;	\
			sibdir = _RB2_GET_RDIFF(gpar, elmdir, field);			\
			_RB2_SET_CHILD(gpar, elmdir, _RB2_PTR(child), field);		\
			_RB2_SET_RDIFF(gpar, elmdir, sibdir, field);			\
		}									\
		return (child);								\
	} while ((parent = gpar) != NULL);						\
	return (NULL);									\
}											\
											\
/* Inserts a node into the RB tree */							\
attr struct type *									\
name##_RB2_INSERT_FINISH(struct name *head, struct type *parent,			\
    __uintptr_t insdir, struct type *elm)						\
{											\
	/*struct type *tmp = NULL;*/							\
	__uintptr_t rdiff = 0;								\
											\
	_RB2_SET_PARENT(elm, parent, field);						\
	rdiff = _RB2_GET_RDIFF(parent, insdir, field);					\
	_RB2_SET_CHILD(parent, insdir, elm, field);					\
	_RB2_SET_RDIFF(parent, insdir, rdiff, field);					\
	name##_RB2_INSERT_BALANCE(head, parent, elm);					\
	return (NULL);									\
}											\
											\
/* Inserts a node into the RB tree */							\
attr struct type *									\
name##_RB2_INSERT(struct name *head, struct type *elm)					\
{											\
	struct type *parent, *tmp;							\
	__typeof(cmp(NULL, NULL)) comp;							\
	__uintptr_t insdir;								\
											\
	_RB2_STACK_CLEAR(head);								\
	_RB2_STACK_PUSH(head, NULL);							\
	_RB2_SET_CHILD(elm, _RB2_LDIR, NULL, field);					\
	_RB2_SET_CHILD(elm, _RB2_RDIR, NULL, field);					\
	tmp = RB2_ROOT(head);								\
	if (tmp == NULL) {								\
		RB2_ROOT(head) = elm;							\
		return (NULL);								\
	}										\
	while (tmp) {									\
		parent = tmp;								\
		comp = cmp(elm, tmp);							\
		if (comp < 0) {								\
			tmp = RB2_LEFT(tmp, field);					\
			insdir = _RB2_LDIR;						\
		}									\
		else if (comp > 0) {							\
			tmp = RB2_RIGHT(tmp, field);					\
			insdir = _RB2_RDIR;						\
		}									\
		else									\
			return (parent);						\
		_RB2_STACK_PUSH(head, parent);						\
	}										\
	/* the stack contains all the nodes upto and including parent */		\
	_RB2_STACK_POP(head, parent);							\
	return (name##_RB2_INSERT_FINISH(head, parent, insdir, elm));			\
}											\

#ifdef RB2_SMALL
#define _RB2_GENERATE_CACHE(name, type, field, cmp, attr)				\
											\
attr struct type *									\
name##_RB2_CACHE(struct name *head, struct type *elm)					\
{											\
	struct type *tmp = RB2_ROOT(head);						\
	__typeof(cmp(NULL, NULL)) comp;							\
	_RB2_STACK_CLEAR(head);								\
	_RB2_STACK_PUSH(head, NULL);							\
	while (tmp) {									\
		_RB2_STACK_PUSH(head, tmp);						\
		comp = cmp(elm, tmp);							\
		if (comp < 0)								\
			tmp = RB2_LEFT(tmp, field);					\
		else if (comp > 0)							\
			tmp = RB2_RIGHT(tmp, field);					\
		else									\
			return (tmp);							\
	}										\
	return (NULL);									\
}
#else
#define _RB2_GENERATE_CACHE(name, type, field, cmp, attr)
#endif

#define _RB2_GENERATE_FIND(name, type, field, cmp, attr)				\
											\
attr struct type *									\
name##_RB2_FIND(struct name *head, struct type *elm)					\
{											\
	struct type *tmp = RB2_ROOT(head);						\
	__typeof(cmp(NULL, NULL)) comp;							\
	while (tmp) {									\
		comp = cmp(elm, tmp);							\
		if (comp < 0)								\
			tmp = RB2_LEFT(tmp, field);					\
		else if (comp > 0)							\
			tmp = RB2_RIGHT(tmp, field);					\
		else									\
			return (tmp);							\
	}										\
	return (NULL);									\
}											\
											\
attr struct type *									\
name##_RB2_NFIND(struct name *head, struct type *elm)					\
{											\
	struct type *tmp = RB2_ROOT(head);						\
	struct type *res = NULL;							\
	__typeof(cmp(NULL, NULL)) comp;							\
	while (tmp) {									\
		comp = cmp(elm, tmp);							\
		if (comp < 0) {								\
			res = tmp;							\
			tmp = RB2_LEFT(tmp, field);					\
		}									\
		else if (comp > 0)							\
			tmp = RB2_RIGHT(tmp, field);					\
		else									\
			return (tmp);							\
	}										\
	return (res);									\
}											\
											\
attr struct type *									\
name##_RB2_PFIND(struct name *head, struct type *elm)					\
{											\
	struct type *tmp = RB2_ROOT(head);						\
	struct type *res = NULL;							\
	__typeof(cmp(NULL, NULL)) comp;							\
	while (tmp) {									\
		comp = cmp(elm, tmp);							\
		if (comp > 0) {								\
			res = tmp;							\
			tmp = RB2_RIGHT(tmp, field);					\
		}									\
		else if (comp < 0)							\
			tmp = RB2_LEFT(tmp, field);					\
		else									\
			return (tmp);							\
	}										\
	return (res);									\
}

#define _RB2_GENERATE_MINMAX(name, type, field, cmp, attr)				\
											\
attr struct type *									\
name##_RB2_MINMAX(struct name *head, int dir)						\
{											\
	struct type *tmp = RB2_ROOT(head);						\
	struct type *parent = NULL;							\
	while (tmp) {									\
		parent = tmp;								\
		tmp = _RB2_GET_CHILD(tmp, dir, field);					\
	}										\
	return (parent);								\
}


/*
 * When doing a balancing of the tree after a removal, lets check when we are looking
 * at the edge 'elm' to its 'parent'.
 * For now, assume that 'elm' has already been demoted.
 * Now there are two possibilities:
 * 1) if 'elm' has rank difference 2 with 'parent', or it is the root node, we are done
 * 2) if 'elm' has rank difference 3 with 'parent', we have a few cases
 *
 * 2.1) the sibling of 'elm' has rank difference 2 with 'parent' then we demote the
 *       'parent'. And continue recursively from parent.
 *
 *                gpar                          gpar
 *                 /                             /
 *               1/2                           2/3
 *               /                             /
 *          parent           -->           parent
 *          /    \                         /    \
 *         3      2                       2      1
 *        /        \                     /        \
 *      elm         sibling            elm        sibling
 *      /\            /\               /\           /\
 *      --            --               --           --
 *
 * 2.2) the sibling of 'elm' has rank difference 1 with 'parent', then we need to do
 *       rotations, based on the children of the sibling of elm.
 *
 * 2.2a) Both children of sibling have rdiff 2. Then we demote both parent and sibling
 *       and continue recursively from parent.
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
 *  2.2b) the rdiff 1 child of 'sibling', 'c', is in the same direction as 'sibling' is wrt
 *       to 'parent'. We do a single rotation (with rank changes) and we are done.
 *
 *                gpar                          gpar                          gpar
 *                 /                             /                             /
 *               1/2                           1/2        if                 1/2
 *               /                             /      parent->c == 2         /
 *          parent           -->           sibling        -->            sibling 
 *          /    \                        1/    \                       2/    \
 *         3      1                     parent   2                   parent    2
 *        /        \                   2/   \     \                  1/   \1    \
 *      elm         sibling           elm    c     d                elm    c     d
 *      /\           /   \1           /\                            /\
 *      --          c     d           --                            --
 *
 * 2.2c) the rdiff 1 child of 'sibling', 'c', is in the opposite direction as 'sibling' is wrt
 *      to 'parent'. We do a double rotation (with rank changes) and we are done.
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

#define _RB2_GENERATE_REMOVE(name, type, field, cmp, attr)				\
											\
attr struct type *									\
name##_RB2_REMOVE_BALANCE(struct name *head, struct type *parent,			\
	struct type *elm)								\
{											\
	struct type *gpar, *sibling;							\
	__uintptr_t elmdir, sibdir, ssdiff, sodiff;					\
	int extend;									\
											\
	_RB2_ASSERT(parent != NULL);							\
	gpar = NULL;									\
	sibling = NULL;									\
	if (RB2_RIGHT(parent, field) == NULL && RB2_LEFT(parent, field) == NULL) {	\
		_RB2_SET_CHILD(parent, _RB2_LDIR, NULL, field);				\
		_RB2_SET_CHILD(parent, _RB2_RDIR, NULL, field);				\
		elm = parent;								\
		_RB2_STACK_POP(head, parent);						\
		_RB2_GET_PARENT(parent, parent, field);					\
		if (parent == NULL) {							\
			return (NULL);							\
		}									\
	}										\
	do {										\
		_RB2_STACK_POP(head, gpar);						\
		_RB2_GET_PARENT(parent, gpar, field);					\
		elmdir = RB2_LEFT(parent, field) == elm ? _RB2_LDIR : _RB2_RDIR;	\
		if (_RB2_GET_RDIFF(parent, elmdir, field) == 0) {			\
			/* case (1) */							\
			_RB2_FLIP_RDIFF(parent, elmdir, field);				\
			return (NULL);							\
		}									\
		/* case 2 */								\
		sibdir = _RB2_ODIR(elmdir);						\
		if (_RB2_GET_RDIFF(parent, sibdir, field)) {				\
			/* case 2.1 */							\
			_RB2_FLIP_RDIFF(parent, sibdir, field);				\
			continue;							\
		}									\
		/* case 2.2 */								\
		sibling = _RB2_PTR(_RB2_GET_CHILD(parent, sibdir, field));		\
		_RB2_ASSERT(sibling != NULL);						\
		ssdiff = _RB2_GET_RDIFF(sibling, elmdir, field);			\
		sodiff = _RB2_GET_RDIFF(sibling, sibdir, field);			\
		if (ssdiff && sodiff) {							\
			/* case 2.2a */							\
			_RB2_FLIP_RDIFF(sibling, elmdir, field);			\
			_RB2_FLIP_RDIFF(sibling, sibdir, field);			\
			continue;							\
		}									\
		extend = 0;								\
		if (sodiff) {								\
			/* case 2.2c */							\
			_RB2_FLIP_RDIFF(sibling, sibdir, field);			\
			_RB2_FLIP_RDIFF(parent, elmdir, field);				\
			elm = _RB2_PTR(_RB2_GET_CHILD(sibling, elmdir, field));		\
			_RB2_ROTATE(sibling, elm, sibdir, field);			\
			_RB2_SET_RDIFF(elm, sibdir, 1, field);				\
			extend = 1;							\
		} else {								\
			/* case 2.2b */							\
			_RB2_FLIP_RDIFF(sibling, sibdir, field);			\
			if (ssdiff) {							\
				_RB2_FLIP_RDIFF(sibling, elmdir, field);		\
				_RB2_FLIP_RDIFF(parent, elmdir, field);			\
				extend = 1;						\
			}								\
			_RB2_FLIP_RDIFF(parent, sibdir, field);				\
			elm = sibling;							\
		}									\
		_RB2_ROTATE(parent, elm, elmdir, field);				\
		_RB2_SET_PARENT(elm, gpar, field);					\
		_RB2_SWAP_CHILD_OR_ROOT(head, gpar, parent, elm, field);		\
		if (extend) {								\
			_RB2_SET_RDIFF(elm, elmdir, 1, field);				\
		}									\
		return (parent);							\
	} while ((elm = parent, (parent = gpar) != NULL));				\
	return (NULL);									\
}											\
											\
attr struct type *									\
name##_RB2_REMOVE_START(struct name *head, struct type *elm)				\
{											\
	/* elm is a node in the tree and the stack contains the parent of elm */	\
	struct type *parent, *opar, *child, *rmin;					\
	__uintptr_t elmdir;								\
	size_t sz;									\
											\
	opar = NULL;									\
	_RB2_STACK_TOP(head, opar);							\
	_RB2_GET_PARENT(child, opar, field);						\
											\
	/* first find the element to swap with oelm */					\
	child = _RB2_GET_CHILD(elm, _RB2_LDIR, field);					\
	rmin = RB2_RIGHT(elm, field);							\
	if (rmin == NULL || _RB2_PTR(child) == NULL) {					\
		rmin = child = (rmin == NULL ? _RB2_PTR(child) : rmin);			\
		parent = opar;								\
	}										\
	else {										\
		_RB2_STACK_PUSH(head, elm);						\
		_RB2_STACK_SIZE(head, &sz);						\
		parent = rmin;								\
		while (RB2_LEFT(rmin, field)) {						\
			_RB2_STACK_PUSH(head, rmin);					\
			rmin = RB2_LEFT(rmin, field);					\
		}									\
		_RB2_SET_PARENT(child, rmin, field);					\
		_RB2_SET_CHILD(rmin, _RB2_LDIR, child, field);				\
		child = _RB2_GET_CHILD(rmin, _RB2_RDIR, field);				\
		if (parent != rmin) {							\
			_RB2_SET_PARENT(parent, rmin, field);				\
			_RB2_SET_CHILD(rmin, _RB2_RDIR, _RB2_GET_CHILD(elm, _RB2_RDIR, field), field);	\
			_RB2_GET_PARENT(rmin, parent, field);				\
			_RB2_STACK_POP(head, parent);					\
			if (parent != NULL) {						\
				_RB2_BITS(_RB2_GET_CHILD(parent, _RB2_LDIR, field)) ^= _RB2_BITS(child) ^ _RB2_BITS(rmin);	\
			}								\
			_RB2_STACK_SET(head, sz - 1, rmin);				\
		} else {								\
			_RB2_STACK_SET(head, sz - 1, NULL);				\
		}									\
		_RB2_SET_RDIFF(rmin, _RB2_RDIR, _RB2_GET_RDIFF(elm, _RB2_RDIR, field), field);	\
		_RB2_SET_PARENT(rmin, opar, field);					\
		child = _RB2_PTR(child);						\
	}										\
	if (opar == NULL) {								\
		RB2_ROOT(head) = rmin;							\
	}										\
	else {										\
		_RB2_SET_PARENT(rmin, opar, field);					\
		elmdir = RB2_LEFT(opar, field) == elm ? _RB2_LDIR : _RB2_RDIR;		\
		_RB2_BITS(_RB2_GET_CHILD(opar, elmdir, field)) ^= _RB2_BITS(elm) ^ _RB2_BITS(rmin);	\
	}										\
	if (child != NULL) {								\
		_RB2_SET_PARENT(child, parent, field);					\
	}										\
	if (parent != NULL) {								\
		name##_RB2_REMOVE_BALANCE(head, parent, child);				\
	}										\
	return (elm);									\
}											\
											\
attr struct type *									\
name##_RB2_REMOVE(struct name *head, struct type *elm)					\
{											\
	struct type *telm;								\
											\
	telm = name##_RB2_CACHE(head, elm);						\
	if (telm == NULL)								\
		return (NULL);								\
	_RB2_STACK_POP(head, telm);							\
	_RB2_ASSERT((cmp(telm, elm)) == 0);						\
	return (name##_RB2_REMOVE_START(head, telm));					\
}

#define RB2_GENERATE(name, type, field, cmp)						\
	_RB2_GENERATE_INTERNAL(name, type, field, cmp,)

#define RB2_GENERATE_STATIC(name, type, field, cmp)					\
	_RB2_GENERATE_INTERNAL(name, type, field, cmp, __attribute__((__unused__)) static)

#define _RB2_GENERATE_INTERNAL(name, type, field, cmp, attr)				\
	_RB2_GENERATE_RANK(name, type, field, cmp, attr)				\
	_RB2_GENERATE_INSERT(name, type, field, cmp, attr)				\
	_RB2_GENERATE_REMOVE(name, type, field, cmp, attr)				\
	_RB2_GENERATE_CACHE(name, type, field, cmp, attr)				\
	_RB2_GENERATE_FIND(name, type, field, cmp, attr)				\
	_RB2_GENERATE_MINMAX(name, type, field, cmp, attr)

#define RB2_PROTOTYPE(name, type, field, cmp)						\
	_RB2_PROTOTYPE_INTERNAL(name, type, field, cmp,)

#define RB2_PROTOTYPE_STATIC(name, type, field, cmp)					\
	_RB2_PROTOTYPE_INTERNAL(name, type, field, cmp, __attribute__((__unused__)) static)

#define _RB2_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)			\
int			 name##_RB2_RANK(const struct type *);			\
attr struct type	*name##_RB2_INSERT(struct name *, struct type *);	\
attr struct type	*name##_RB2_REMOVE(struct name *, struct type *);	\
attr struct type	*name##_RB2_CACHE(struct name *, struct type *);	\
attr struct type	*name##_RB2_FIND(struct name *, struct type *);		\
attr struct type	*name##_RB2_NFIND(struct name *, struct type *);	\
attr struct type	*name##_RB2_PFIND(struct name *, struct type *);	\
attr struct type	*name##_RB2_MINMAX(struct name *, int);

#define RB2_RANK(name, head)		name##_RB2_RANK(head)
#define RB2_INSERT(name, head, elm)	name##_RB2_INSERT(head, elm)
#define RB2_REMOVE(name, head, elm)	name##_RB2_REMOVE(head, elm)
#define RB2_FIND(name, head, elm)	name##_RB2_FIND(head, elm)
#define RB2_NFIND(name, head, elm)	name##_RB2_NFIND(head, elm)
#define RB2_PFIND(name, head, elm)	name##_RB2_PFIND(head, elm)
#define RB2_MIN(name, head)		name##_RB2_MINMAX(head, _RB2_LDIR)
#define RB2_MAX(name, head)		name##_RB2_MINMAX(head, _RB2_RDIR)


#endif /* _SYS_RBTREE_H_ */
