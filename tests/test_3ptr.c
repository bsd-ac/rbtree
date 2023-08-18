/*	$OpenBSD: rb-test.c,v 1.4 2008/04/13 00:22:17 djm Exp $	*/
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

#include <sys/types.h>
//#include <sys/tree.h>
#include <unistd.h>
#include <stdio.h>
#include <err.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#include "rb.h"

#define IMAX(a, b) ((a) > (b) ? (a) : (b))

struct node {
	RB_ENTRY(node) node;
	int key;
	size_t height;
	size_t size;
};

RB_HEAD(tree, node) root;

static int
compare(struct node *a, struct node *b)
{
	if (a->key < b->key) return (-1);
	else if (a->key > b->key) return (1);
	return (0);
}
/*
#undef RB_AUGMENT
#define RB_AUGMENT(elm) do {						\
if(!(elm)) break;							\
elm->size = 1;								\
elm->height = 0;							\
if ((RB_LEFT(elm, node))) {						\
	elm->size += (RB_LEFT(elm, node))->size;			\
	elm->height = IMAX((RB_LEFT(elm, node))->height, elm->height);	\
}									\
if ((RB_RIGHT(elm, node))) {						\
	elm->size += (RB_RIGHT(elm, node))->size;			\
	elm->height = IMAX((RB_RIGHT(elm, node))->height, elm->height);	\
}									\
elm->height += 1;							\
} while (0)
*/

RB_PROTOTYPE(tree, node, node, compare);

RB_GENERATE(tree, node, node, compare);

#define ITER 15000000
#define MIN 5
#define MAX 100000000

int
main(int argc, char **argv)
{
	struct node *tmp, *ins, *it;
	int i, max, min;
        struct timespec start, end, diff;

	RB_INIT(&root);
        printf("starting sequential insertions\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);

	for (i = 0; i < ITER; i++) {
		tmp = malloc(sizeof(struct node));
		if (tmp == NULL) err(1, "malloc");
		tmp->size = 1;
		tmp->height = 1;
		do {
			tmp->key = i; //arc4random_uniform(MAX-MIN);
			tmp->key += MIN;
		} while (RB_FIND(tree, &root, tmp) != NULL);
		if (i == 0)
			max = min = tmp->key;
		else {
			if (tmp->key > max)
				max = tmp->key;
			if (tmp->key < min)
				min = tmp->key;
		}
		if (RB_INSERT(tree, &root, tmp) != NULL)
			errx(1, "RB_INSERT failed");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential insertions in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);
	/*
	ins = RB_ROOT(&root);
	if (ins->size != ITER)
		errx(1, "size does not match");
	*/
	printf("getting min\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MIN(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done getting min in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);
	
	if (ins->key != min)
		errx(1, "min does not match");
	tmp = ins;
	printf("getting max\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MAX(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done getting min in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);
	if (ins->key != max)
		errx(1, "max does not match");

	if (RB_REMOVE(tree, &root, tmp) != tmp)
		errx(1, "RB_REMOVE failed");
	/*
	if ((RB_ROOT(&root))->size != ITER - 1)
		errx(1, "RB_REMOVE initial size error");
	*/
	printf("doing root removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER - 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
		free(tmp);
		/*
		if (!(RB_EMPTY(&root)) && (RB_ROOT(&root))->size != ITER - 2 - i)
			errx(1, "RB_REMOVE size error");
		*/
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done root removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	exit(0);
}
