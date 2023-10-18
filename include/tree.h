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

#ifndef _SYS_TREE_H_
#define _SYS_TREE_H_

#include <stddef.h>
#include <stdint.h>

/*
 * This code is an implementation of the rank balanced tree with weak AVL
 * conditions. The following paper describes the rank balanced trees in detail:
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
 * at most two rotations, which is an improvement over the standard AVL and
 * Red-Black trees.
 * As a comparison matrix:
 *   Tree type                      Weak AVL       AVL              Red-Black
 *   worst case height              2Log(N)        1.44Log(N)       2Log(N)
 *   height with only insertions    1.44Log(N)     1.44Log(N)       2Log(N)
 *   max rotations after insert     2              O(Log(N))        2
 *   max rotations after delete     2              2                3
 * 
 * For each node we store the left, right and parent pointers.
 * We assume that a pointer to a struct is aligned to multiple of 4 bytes, which
 * leaves the last two bits free for our use. The last two bits of the parent
 * are used to store the rank difference between the node and its children.
 * The convention used here is that the rank difference = 1 + child bit.
 */

#include <stdint.h>

/*
 * debug macros
 */
#ifndef _RB_DIAGNOSTIC
#define _RB_ASSERT(x)		do {} while (0)
#else
#include <assert.h>
#define _RB_ASSERT(x)		assert(x)
#endif

/* 
 * internal use macros
 */
#define _RB_LOWMASK					((uintptr_t)3U)
#define _RB_PTR(elm)					(__typeof(elm))((uintptr_t)(elm) & ~_RB_LOWMASK)

#define _RB_PDIR					((uintptr_t)0U)
#define _RB_LDIR					((uintptr_t)1U)
#define _RB_RDIR					((uintptr_t)2U)
#define _RB_ODIR(dir)					((dir) ^ 3U)

#define RB_ENTRY(type)					\
struct {						\
	/* parent, left, right */			\
	struct type *child[3];				\
}

#define RB_HEAD(name, type)				\
struct name {						\
	struct type *root;				\
}

#define RB_INITIALIZER(root)				\
{ NULL }


#define RB_INIT(head) do {				\
(head)->root = NULL;					\
} while (0)

#define RB_ROOT(head)					(head)->root
#define RB_EMPTY(head)					(RB_ROOT(head) == NULL)

/*
 * element macros
 */
#define _RB_GET_CHILD(elm, dir, field)			(elm)->field.child[dir]
#define _RB_SET_CHILD(elm, dir, celm, field) do {	\
_RB_GET_CHILD(elm, dir, field) = (celm);		\
} while (0)
#define _RB_UP(elm, field)				_RB_GET_CHILD(elm, _RB_PDIR, field)
#define _RB_REPLACE_PARENT(elm, pelm, field)		_RB_SET_CHILD(elm, _RB_PDIR, pelm, field)
#define _RB_COPY_PARENT(elm, nelm, field)		_RB_REPLACE_PARENT(nelm, _RB_UP(elm, field), field)
#define _RB_SET_PARENT(elm, pelm, field) do {		\
_RB_UP(elm, field) = (__typeof(elm))(((uintptr_t)pelm) | 	\
	    (((uintptr_t)_RB_UP(elm, field)) & _RB_LOWMASK));	\
} while (0)


#define RB_LEFT(elm, field)				_RB_GET_CHILD(elm, _RB_LDIR, field)
#define RB_RIGHT(elm, field)				_RB_GET_CHILD(elm, _RB_RDIR, field)
#define RB_PARENT(elm, field)				_RB_PTR(_RB_GET_CHILD(elm, _RB_PDIR, field))


#define _RB_GET_RDIFF(elm, dir, field)			(((uintptr_t)_RB_UP(elm, field)) & dir)
#define _RB_GET_RDIFF2(elm, field)			(((uintptr_t)_RB_UP(elm, field)) & _RB_LOWMASK)
#define _RB_FLIP_RDIFF(elm, dir, field)			\
_RB_REPLACE_PARENT(elm, ((__typeof(elm))(((uintptr_t)_RB_UP(elm, field)) ^ dir)), field)
#define _RB_FLIP_RDIFF2(elm, field)			\
_RB_REPLACE_PARENT(elm, ((__typeof(elm))(((uintptr_t)_RB_UP(elm, field)) ^ _RB_LDIR ^ _RB_RDIR)), field)
#define _RB_SET_RDIFF0(elm, dir, field)			\
_RB_REPLACE_PARENT(elm, ((__typeof(elm))(((uintptr_t)_RB_UP(elm, field)) & ~dir)), field)
#define _RB_SET_RDIFF1(elm, dir, field)			\
_RB_REPLACE_PARENT(elm, ((__typeof(elm))(((uintptr_t)_RB_UP(elm, field)) | dir)), field)
#define _RB_SET_RDIFF11(elm, field)			\
_RB_REPLACE_PARENT(elm, ((__typeof(elm))(((uintptr_t)_RB_UP(elm, field)) | _RB_LDIR | _RB_RDIR)), field)
#define _RB_SET_RDIFF00(elm, field)			\
_RB_REPLACE_PARENT(elm, RB_PARENT(elm, field), field)


/*
 * RB_AUGMENT_CHECK should only return true when the update changes the node data,
 * so that updating can be stopped short of the root when it returns false.
 */
#ifndef RB_AUGMENT_CHECK
#ifndef RB_AUGMENT
#define RB_AUGMENT_CHECK(x) (0)
#else
#define RB_AUGMENT_CHECK(x) (RB_AUGMENT(x), 1)
#endif
#endif

#define _RB_AUGMENT_WALK(elm, match, field) do {	\
if (match == elm)					\
	match = NULL;					\
} while (RB_AUGMENT_CHECK(elm) &&			\
	(elm = RB_PARENT(elm, field)) != NULL)


/*
 *      elm            celm
 *      / \            / \
 *     c1  celm       elm gc2
 *         / \        / \
 *      gc1   gc2    c1 gc1
 */
#define _RB_ROTATE(elm, celm, dir, field) do {				\
if ((_RB_GET_CHILD(elm, _RB_ODIR(dir), field) = _RB_GET_CHILD(celm, dir, field)) != NULL)	\
	_RB_SET_PARENT(_RB_GET_CHILD(celm, dir, field), elm, field);	\
_RB_SET_CHILD(celm, dir, elm, field);					\
_RB_SET_PARENT(elm, celm, field);					\
} while (0)

#define _RB_SWAP_CHILD_OR_ROOT(head, elm, oelm, nelm, field) do {	\
if (elm == NULL)							\
	RB_ROOT(head) = nelm;						\
else									\
	_RB_SET_CHILD(elm, (RB_LEFT(elm, field) == (oelm) ? _RB_LDIR : _RB_RDIR), nelm, field);	\
} while (0)


#define _RB_GENERATE_RANK(name, type, field, cmp, attr)			\
attr int								\
name##_RB_RANK(const struct type *elm)					\
{									\
	int lrank, rrank;						\
	if (elm == NULL)						\
		return (-1);						\
	lrank =  name##_RB_RANK(RB_LEFT(elm, field));			\
	if (lrank == -2)						\
		return (-2);						\
	rrank =  name##_RB_RANK(RB_RIGHT(elm, field));			\
	if (rrank == -2)						\
		return (-2);						\
	lrank += (_RB_GET_RDIFF(elm, _RB_LDIR, field) == 0) ? 1 : 2;	\
	rrank += (_RB_GET_RDIFF(elm, _RB_RDIR, field) == 0) ? 1 : 2;	\
	if (lrank != rrank)						\
		return (-2);						\
	return (lrank);							\
}


#define _RB_GENERATE_MINMAX(name, type, field, cmp, attr)		\
									\
attr struct type *							\
name##_RB_MINMAX(struct name *head, int dir)				\
{									\
	struct type *tmp = RB_ROOT(head);				\
	struct type *parent = NULL;					\
	while (tmp) {							\
		parent = tmp;						\
		tmp = _RB_GET_CHILD(tmp, dir, field);			\
	}								\
	return (parent);						\
}


#define _RB_GENERATE_ITERATE(name, type, field, cmp, attr)		\
									\
attr struct type *							\
name##_RB_NEXT(struct type *elm)					\
{									\
	struct type *parent = NULL;					\
									\
	if (RB_RIGHT(elm, field)) {					\
		elm = RB_RIGHT(elm, field);				\
		while (RB_LEFT(elm, field))				\
			elm = RB_LEFT(elm, field);			\
	} else {							\
		parent = RB_PARENT(elm, field);				\
		while (parent && elm == RB_RIGHT(parent, field)) {	\
			elm = parent;					\
			parent = RB_PARENT(parent, field);		\
		}							\
		elm = parent;						\
	}								\
	return (elm);							\
}									\
									\
attr struct type *							\
name##_RB_PREV(struct type *elm)					\
{									\
	struct type *parent = NULL;					\
									\
	if (RB_LEFT(elm, field)) {					\
		elm = RB_LEFT(elm, field);				\
		while (RB_RIGHT(elm, field))				\
			elm = RB_RIGHT(elm, field);			\
	} else {							\
		parent = RB_PARENT(elm, field);				\
		while (parent && elm == RB_LEFT(parent, field)) {	\
			elm = parent;					\
			parent = RB_PARENT(parent, field);		\
		}							\
		elm = parent;						\
	}								\
	return (elm);							\
}


#define _RB_GENERATE_FIND(name, type, field, cmp, attr)			\
									\
attr struct type *							\
name##_RB_FIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = RB_ROOT(head);				\
	__typeof(cmp(NULL, NULL)) comp;					\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0)						\
			tmp = RB_LEFT(tmp, field);			\
		else if (comp > 0)					\
			tmp = RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (NULL);							\
}									\
									\
attr struct type *							\
name##_RB_NFIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = RB_ROOT(head);				\
	struct type *res = NULL;					\
	__typeof(cmp(NULL, NULL)) comp;					\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp < 0) {						\
			res = tmp;					\
			tmp = RB_LEFT(tmp, field);			\
		}							\
		else if (comp > 0)					\
			tmp = RB_RIGHT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (res);							\
}									\
									\
attr struct type *							\
name##_RB_PFIND(struct name *head, struct type *elm)			\
{									\
	struct type *tmp = RB_ROOT(head);				\
	struct type *res = NULL;					\
	__typeof(cmp(NULL, NULL)) comp;					\
	while (tmp) {							\
		comp = cmp(elm, tmp);					\
		if (comp > 0) {						\
			res = tmp;					\
			tmp = RB_RIGHT(tmp, field);			\
		}							\
		else if (comp < 0)					\
			tmp = RB_LEFT(tmp, field);			\
		else							\
			return (tmp);					\
	}								\
	return (res);							\
}


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
#define _RB_GENERATE_REMOVE(name, type, field, cmp, attr)		\
									\
attr struct type *							\
name##_RB_REMOVE_BALANCE(struct name *head, struct type *parent,	\
    struct type *elm)							\
{									\
	struct type *gpar, *sibling;					\
	uintptr_t elmdir, sibdir, ssdiff, sodiff;			\
									\
	gpar = NULL;							\
	sibling = NULL;							\
	if (RB_RIGHT(parent, field) == NULL && RB_LEFT(parent, field) == NULL) {	\
		_RB_SET_RDIFF00(parent, field);				\
		elm = parent;						\
		if ((parent = RB_PARENT(elm, field)) == NULL) {		\
			return (NULL);					\
		}							\
	}								\
	do {								\
		_RB_ASSERT(parent != NULL);				\
		gpar = RB_PARENT(parent, field);			\
		elmdir = RB_LEFT(parent, field) == elm ? _RB_LDIR : _RB_RDIR;	\
		if (_RB_GET_RDIFF(parent, elmdir, field) == 0) {	\
			/* case (1) */					\
			_RB_FLIP_RDIFF(parent, elmdir, field);		\
			return (NULL);					\
		}							\
		/* case 2 */						\
		sibdir = _RB_ODIR(elmdir);				\
		if (_RB_GET_RDIFF(parent, sibdir, field)) {		\
			/* case 2.1 */					\
			_RB_FLIP_RDIFF(parent, sibdir, field);		\
			continue;					\
		}							\
		/* case 2.2 */						\
		sibling = _RB_GET_CHILD(parent, sibdir, field);		\
		_RB_ASSERT(sibling != NULL);				\
		ssdiff = _RB_GET_RDIFF(sibling, elmdir, field);		\
		sodiff = _RB_GET_RDIFF(sibling, sibdir, field);		\
		_RB_FLIP_RDIFF(sibling, sibdir, field);			\
		if (ssdiff && sodiff) {					\
			/* case 2.2a */					\
			_RB_FLIP_RDIFF(sibling, elmdir, field);		\
			continue;					\
		}							\
		if (sodiff) {						\
			/* case 2.2c */					\
			elm = _RB_GET_CHILD(sibling, elmdir, field);	\
			_RB_ROTATE(sibling, elm, sibdir, field);	\
			_RB_FLIP_RDIFF(parent, elmdir, field);		\
			if (_RB_GET_RDIFF(elm, elmdir, field))		\
				_RB_FLIP_RDIFF(parent, sibdir, field);	\
			if (_RB_GET_RDIFF(elm, sibdir, field))		\
				_RB_FLIP_RDIFF(sibling, elmdir, field);	\
			_RB_SET_RDIFF11(elm, field);			\
		} else {						\
			/* case 2.2b */					\
			if (ssdiff) {					\
				_RB_SET_RDIFF11(sibling, field);	\
				_RB_SET_RDIFF00(parent, field);		\
			}						\
			elm = sibling;					\
		}							\
		_RB_ROTATE(parent, elm, elmdir, field);			\
		_RB_SET_PARENT(elm, gpar, field);			\
		_RB_SWAP_CHILD_OR_ROOT(head, gpar, parent, elm, field);	\
		if (elm != sibling)					\
			(void)RB_AUGMENT_CHECK(sibling);		\
		return (elm);						\
	} while ((elm = parent, (parent = gpar) != NULL));		\
	return (elm);							\
}									\
									\
attr struct type *							\
name##_RB_REMOVE(struct name *head, struct type *elm)			\
{									\
	struct type *parent, *opar, *child, *rmin;			\
									\
	/* first find the element to swap with elm */			\
	child = RB_LEFT(elm, field);					\
	rmin = RB_RIGHT(elm, field);					\
	if (rmin == NULL || child == NULL) {				\
		rmin = child = (rmin == NULL ? child : rmin);		\
		parent = opar = RB_PARENT(elm, field);			\
	}								\
	else {								\
		parent = rmin;						\
		while (RB_LEFT(rmin, field)) {				\
			rmin = RB_LEFT(rmin, field);			\
		}							\
		_RB_SET_PARENT(child, rmin, field);			\
		_RB_SET_CHILD(rmin, _RB_LDIR, child, field);		\
		child = RB_RIGHT(rmin, field);				\
		if (parent != rmin) {					\
			_RB_SET_PARENT(parent, rmin, field);		\
			_RB_SET_CHILD(rmin, _RB_RDIR, parent, field);	\
			parent = RB_PARENT(rmin, field);		\
			_RB_SET_CHILD(parent, _RB_LDIR, child, field);	\
		}							\
		_RB_COPY_PARENT(elm, rmin, field);			\
		opar = RB_PARENT(elm, field);				\
	}								\
	_RB_SWAP_CHILD_OR_ROOT(head, opar, elm, rmin, field);		\
	if (child != NULL) {						\
		_RB_REPLACE_PARENT(child, parent, field);		\
	}								\
	if (parent != NULL) {						\
		opar = name##_RB_REMOVE_BALANCE(head, parent, child);	\
		if (parent == rmin && RB_LEFT(parent, field) == NULL) {	\
			opar = NULL;					\
			parent = RB_PARENT(parent, field);		\
		}							\
		_RB_AUGMENT_WALK(parent, opar, field);			\
		if (opar != NULL) {					\
			(void)RB_AUGMENT_CHECK(opar);			\
			(void)RB_AUGMENT_CHECK(RB_PARENT(opar, field));	\
		}							\
	}								\
	return (elm);							\
}


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
#define _RB_GENERATE_INSERT(name, type, field, cmp, attr)		\
									\
attr struct type *							\
name##_RB_INSERT_BALANCE(struct name *head, struct type *parent,	\
    struct type *elm)							\
{									\
	struct type *child, *child_up, *gpar;				\
	uintptr_t elmdir, sibdir;					\
									\
	child = NULL;							\
	gpar = NULL;							\
	do {								\
		/* elm has not been promoted yet */			\
		elmdir = RB_LEFT(parent, field) == elm ? _RB_LDIR : _RB_RDIR;	\
		if (_RB_GET_RDIFF(parent, elmdir, field)) {		\
			/* case (1) */					\
			_RB_FLIP_RDIFF(parent, elmdir, field);		\
			return (elm);					\
		}							\
		/* case (2) */						\
		gpar = RB_PARENT(parent, field);			\
		sibdir = _RB_ODIR(elmdir);				\
		_RB_FLIP_RDIFF(parent, sibdir, field);			\
		if (_RB_GET_RDIFF(parent, sibdir, field)) {		\
			/* case (2.1) */				\
			child = elm;					\
			elm = parent;					\
			continue;					\
		}							\
		/* we can only reach this point if we are in the	\
		 * second or greater iteration of the while loop	\
		 * which means that 'child' has been populated		\
		 */							\
		_RB_SET_RDIFF00(parent, field);				\
		/* case (2.2) */					\
		if (_RB_GET_RDIFF(elm, sibdir, field) == 0) {		\
			/* case (2.2b) */				\
			_RB_ROTATE(elm, child, elmdir, field);		\
			if (_RB_GET_RDIFF(child, sibdir, field))	\
				_RB_FLIP_RDIFF(parent, elmdir, field);	\
			if (_RB_GET_RDIFF(child, elmdir, field))	\
				_RB_FLIP_RDIFF2(elm, field);		\
			else						\
				_RB_FLIP_RDIFF(elm, elmdir, field);	\
			if (_RB_GET_RDIFF2(child, field) == 0)		\
				elm = child;				\
		} else {						\
			/* case (2.2a) */				\
			child = elm;					\
		}							\
		_RB_ROTATE(parent, child, sibdir, field);		\
		_RB_REPLACE_PARENT(child, gpar, field);			\
		_RB_SWAP_CHILD_OR_ROOT(head, gpar, parent, child, field);	\
		if (elm != child)					\
			(void)RB_AUGMENT_CHECK(elm);			\
		(void)RB_AUGMENT_CHECK(parent);				\
		return (child);						\
	} while ((parent = gpar) != NULL);				\
	return (NULL);							\
}									\
									\
/* Inserts a node into the RB tree */					\
attr struct type *							\
name##_RB_INSERT_FINISH(struct name *head, struct type *parent,		\
    struct type **tmpp, struct type *elm)				\
{									\
	struct type *tmp = elm;						\
	_RB_SET_CHILD(elm, _RB_LDIR, NULL, field);			\
	_RB_SET_CHILD(elm, _RB_RDIR, NULL, field);			\
	_RB_REPLACE_PARENT(elm, parent, field);				\
	*tmpp = elm;							\
	if (parent != NULL)						\
		tmp = name##_RB_INSERT_BALANCE(head, parent, elm);	\
	_RB_AUGMENT_WALK(elm, tmp, field);				\
	if (tmp != NULL)						\
		(void)RB_AUGMENT_CHECK(tmp);				\
	return (NULL);							\
}									\
									\
/* Inserts a node into the RB tree */					\
attr struct type *							\
name##_RB_INSERT(struct name *head, struct type *elm)			\
{									\
	struct type *tmp;						\
	struct type *parent=NULL;					\
	struct type **tmpp=&RB_ROOT(head);				\
	__typeof(cmp(NULL, NULL)) comp;					\
									\
	while ((tmp = *tmpp) != NULL) {					\
		parent = tmp;						\
		comp = cmp(elm, tmp);					\
		if (comp < 0) {						\
			tmpp = &RB_LEFT(tmp, field);			\
		}							\
		else if (comp > 0) {					\
			tmpp = &RB_RIGHT(tmp, field);			\
		}							\
		else							\
			return (parent);				\
	}								\
	return (name##_RB_INSERT_FINISH(head, parent, tmpp, elm));	\
}									\
									\
attr struct type *							\
name##_RB_INSERT_NEXT(struct name *head, struct type *elm, struct type *next)	\
{									\
	struct type *tmp;						\
	struct type **tmpp;						\
	_RB_ASSERT((cmp)(elm, next) < 0);				\
	if (name##_RB_NEXT(elm) != NULL)				\
		_RB_ASSERT((cmp)(next, name##_RB_NEXT(elm)) < 0);	\
	tmpp = &RB_RIGHT(elm, field);					\
	while ((tmp = *tmpp) != NULL) {					\
		elm = tmp;						\
		tmpp = &RB_LEFT(tmp, field);				\
	}								\
	return name##_RB_INSERT_FINISH(head, elm, tmpp, next);		\
}									\
									\
attr struct type *							\
name##_RB_INSERT_PREV(struct name *head, struct type *elm, struct type *prev)	\
{									\
	struct type *tmp;						\
	struct type **tmpp;						\
	_RB_ASSERT((cmp)(elm, prev) > 0);				\
	if (name##_RB_PREV(elm) != NULL)				\
		_RB_ASSERT((cmp)(prev, name##_RB_PREV(elm)) > 0);	\
									\
	tmpp = &RB_LEFT(elm, field);					\
	while ((tmp = *tmpp) != NULL) {					\
		elm = tmp;						\
		tmpp = &RB_RIGHT(tmp, field);				\
	}								\
	return name##_RB_INSERT_FINISH(head, elm, tmpp, prev);		\
}


#define RB_GENERATE(name, type, field, cmp)				\
	_RB_GENERATE_INTERNAL(name, type, field, cmp,)

#define RB_GENERATE_STATIC(name, type, field, cmp)			\
	_RB_GENERATE_INTERNAL(name, type, field, cmp, __attribute__((__unused__)) static)

#define _RB_GENERATE_INTERNAL(name, type, field, cmp, attr)		\
	_RB_GENERATE_RANK(name, type, field, cmp, attr)			\
	_RB_GENERATE_MINMAX(name, type, field, cmp, attr)		\
	_RB_GENERATE_ITERATE(name, type, field, cmp, attr)		\
	_RB_GENERATE_FIND(name, type, field, cmp, attr)			\
	_RB_GENERATE_REMOVE(name, type, field, cmp, attr)		\
	_RB_GENERATE_INSERT(name, type, field, cmp, attr)


#define RB_PROTOTYPE(name, type, field, cmp)				\
	_RB_PROTOTYPE_INTERNAL(name, type, field, cmp,)

#define RB_PROTOTYPE_STATIC(name, type, field, cmp)			\
	_RB_PROTOTYPE_INTERNAL(name, type, field, cmp, __attribute__((__unused__)) static)

#define _RB_PROTOTYPE_INTERNAL(name, type, field, cmp, attr)		\
attr int		 name##_RB_RANK(const struct type *);		\
attr struct type	*name##_RB_MINMAX(struct name *, int);		\
attr struct type	*name##_RB_NEXT(struct type *);			\
attr struct type	*name##_RB_PREV(struct type *);			\
attr struct type	*name##_RB_FIND(struct name *, struct type *);	\
attr struct type	*name##_RB_NFIND(struct name *, struct type *);	\
attr struct type	*name##_RB_PFIND(struct name *, struct type *);	\
attr struct type	*name##_RB_REMOVE(struct name *, struct type *);	\
attr struct type	*name##_RB_INSERT(struct name *, struct type *);	\
attr struct type	*name##_RB_INSERT_NEXT(struct name *, struct type *, struct type *);	\
attr struct type	*name##_RB_INSERT_PREV(struct name *, struct type *, struct type *);


#define RB_RANK(name, head)			name##_RB_RANK(head)
#define RB_MIN(name, head)			name##_RB_MINMAX(head, _RB_LDIR)
#define RB_MAX(name, head)			name##_RB_MINMAX(head, _RB_RDIR)
#define RB_NEXT(name, head, elm)		name##_RB_NEXT(elm)
#define RB_PREV(name, head, elm)		name##_RB_PREV(elm)
#define RB_FIND(name, head, elm)		name##_RB_FIND(head, elm)
#define RB_NFIND(name, head, elm)		name##_RB_NFIND(head, elm)
#define RB_PFIND(name, head, elm)		name##_RB_PFIND(head, elm)
#define RB_REMOVE(name, head, elm)		name##_RB_REMOVE(head, elm)
#define RB_INSERT(name, head, elm)		name##_RB_INSERT(head, elm)
#define RB_INSERT_NEXT(name, head, elm, next)	name##_RB_INSERT_NEXT(head, elm, next)
#define RB_INSERT_PREV(name, head, elm, prev)	name##_RB_INSERT_PREV(head, elm, prev)


#define RB_FOREACH(x, name, head)					\
	for ((x) = RB_MIN(name, head);					\
	     (x) != NULL;						\
	     (x) = name##_RB_NEXT(x))

#define RB_FOREACH_FROM(x, name, y)					\
	for ((x) = (y);							\
	    ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL);	\
	     (x) = (y))

#define RB_FOREACH_SAFE(x, name, head, y)				\
	for ((x) = RB_MIN(name, head);					\
	    ((x) != NULL) && ((y) = name##_RB_NEXT(x), (x) != NULL);	\
	     (x) = (y))

#define RB_FOREACH_REVERSE(x, name, head)				\
	for ((x) = RB_MAX(name, head);					\
	     (x) != NULL;						\
	     (x) = name##_RB_PREV(x))

#define RB_FOREACH_REVERSE_FROM(x, name, y)				\
	for ((x) = (y);							\
	    ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL);	\
	     (x) = (y))

#define RB_FOREACH_REVERSE_SAFE(x, name, head, y)			\
	for ((x) = RB_MAX(name, head);					\
	    ((x) != NULL) && ((y) = name##_RB_PREV(x), (x) != NULL);	\
	     (x) = (y))

#endif /* _SYS_TREE_H_ */
