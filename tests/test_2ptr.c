#include <sys/time.h>

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define RB_SMALL
//#define RB_DIAGNOSTIC
#include "tree.h"

#define TDEBUGF(fmt, ...)	fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


#ifdef __OpenBSD__
#define SEED_RANDOM srandom_deterministic
#else
#define SEED_RANDOM srandom
#endif

#ifndef timespecsub
#define	timespecsub(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec - (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec - (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec < 0) {				\
			(vsp)->tv_sec--;				\
			(vsp)->tv_nsec += 1000000000L;			\
		}							\
	} while (0)
#endif

int ITER=15000000;
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* declarations */
struct node;
struct tree;
static int compare(const struct node *, const struct node *);
static void print_helper(const struct node *, int);
static void print_tree(const struct tree *);
static int tree_augment(struct node *);
static void mix_operations(int *, int, struct node *, int, int, int, int);

/* definitions */
struct node {
	RB_ENTRY(node) node_link;
	int             key;
	size_t          height;
	size_t          size;
};

RB_HEAD(tree, node);
struct tree root;

//#undef RB_AUGMENT
//#define RB_AUGMENT(elm) tree_augment(elm)

RB_PROTOTYPE(tree, node, node_link, compare)

RB_GENERATE(tree, node, node_link, compare)

int
main()
{
	struct node *tmp, *ins, *nodes;
	int i, r, *perm, *nums;
        struct timespec start, end, diff;

	nodes = malloc((ITER + 5) * sizeof(struct node));
        perm = malloc(sizeof(int) * ITER);
	nums = malloc(sizeof(int) * ITER);

        // for determinism
        SEED_RANDOM(423);

        TDEBUGF("generating a 'random' permutation");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        perm[0] = 0;
	nums[0] = 0;
        for(i = 1; i < ITER; i++) {
		r = random() % i; // arc4random_uniform(i);
                perm[i] = perm[r];
                perm[r] = i;
		nums[i] = i;
        }
        /*
        fprintf(stderr, "{");
        for(int i = 0; i < ITER; i++) {
                fprintf(stderr, "%d, ", perm[i]);
        }
        fprintf(stderr, "}\n");
        int nperm[10] = {2, 4, 9, 7, 8, 3, 0, 1, 6, 5};
        int nperm[6] = {2, 6, 1, 4, 5, 3};
        int nperm[10] = {10, 3, 7, 8, 6, 1, 9, 2, 5, 4};
        int nperm[2] = {0, 1};

        int nperm[100] = {54, 47, 31, 35, 40, 73, 29, 66, 15, 45, 9, 71, 51, 32, 28, 62, 12, 46, 50, 26, 36, 91, 10, 76, 33, 43, 34, 58, 55, 72, 37, 24, 75, 4, 90, 88, 30, 25, 82, 18, 67, 81, 80, 65, 23, 41, 61, 86, 20, 99, 59, 14, 79, 21, 68, 27, 1, 7, 94, 44, 89, 64, 96, 2, 49, 53, 74, 13, 48, 42, 60, 52, 95, 17, 11, 0, 22, 97, 77, 69, 6, 16, 84, 78, 8, 83, 98, 93, 39, 38, 85, 70, 3, 19, 57, 5, 87, 92, 63, 56};
        ITER = 100;
        for(int i = 0; i < ITER; i++){
                perm[i] = nperm[i];
        }
        */

        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done generating a 'random' permutation in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	RB_INIT(&root);

        TDEBUGF("starting random insertions");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done random insertions in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

#ifdef DIAGNOSTIC
	print_tree(RB_ROOT(&root));
#endif

#ifdef DOAUGMENT
	ins = RB_ROOT(&root);
	if (ins->size != ITER + 1)
		errx(1, "size does not match");
#endif
/*
	TDEBUGF("getting min");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MIN(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done getting min in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);
	if (ins->key != 0)
		errx(1, "min does not match");

	TDEBUGF("getting max");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MAX(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done getting max in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);
	if (ins->key != ITER + 5)
		errx(1, "max does not match");
*/
	ins = RB_ROOT(&root);
	if (RB_REMOVE(tree, &root, ins) != ins)
		errx(1, "RB_REMOVE failed");
        //print_tree(&root);
/*
#ifdef DOAUGMENT
	if ((RB_ROOT(&root))->size != ITER)
	  errx(1, "RB_REMOVE initial size error: %zu", (RB_ROOT(&root))->size);
#endif
*/
	TDEBUGF("doing root removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
                //TDEBUGF("removal number %d = %d", i, tmp->key);
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
                //print_tree(&root);
                //int rank = RB_RANK(tree, RB_ROOT(&root));
                //if (rank == -2)
                //        errx(1, "rank error");

#ifdef DOAUGMENT
		if (!(RB_EMPTY(&root)) && (RB_ROOT(&root))->size != ITER - 1 - i)
			errx(1, "RB_REMOVE size error");
#endif
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
/*
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}

        TDEBUGF("starting sequential insertions");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential insertions in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

        TDEBUGF("starting sequential insertions");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential insertions in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing find and remove");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i <= ITER; i++) {
		tmp->key = i;
		ins = RB_FIND(tree, &root, tmp);
		if (ins != NULL)
			if (RB_REMOVE(tree, &root, ins) != ins)
				errx(1, "RB_REMOVE failed: %d", i);
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing 50%% insertions, 50%% lookups");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 2, ITER / 2, 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done operations in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing 20%% insertions, 80%% lookups");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 5, 4 * (ITER / 5), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done operations in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing 10%% insertions, 90%% lookups");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 10, 9 * (ITER / 10), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done operations in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing 5%% insertions, 95%% lookups");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 5 * (ITER / 100), 95 * (ITER / 100), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done operations in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing 2%% insertions, 98%% lookups");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 2 * (ITER / 100), 98 * (ITER / 100), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done operations in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);

	TDEBUGF("doing sequential removals");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done sequential removals in: %lld.%09ld s", diff.tv_sec, diff.tv_nsec);
        */

	free(nodes);
        free(perm);
        free(nums);
	exit(0);
}


static int
compare(const struct node *a, const struct node *b)
{
        return a->key - b->key;
}

static void
print_helper(const struct node *n, int indent)
{
	if (RB_RIGHT(n, node_link))
  		print_helper(RB_RIGHT(n, node_link), indent + 4);
	TDEBUGF("%*s key=%d :: size=%zu :: rank=%d :: rdiff %lu:%lu", indent, "", n->key, n->size, 0, 0, 0);
	if (RB_LEFT(n, node_link))
  		print_helper(RB_LEFT(n, node_link), indent + 4);
}

static void
print_tree(const struct tree *t)
{
	if (RB_ROOT(t)) print_helper(RB_ROOT(t), 0);
}

static int
tree_augment(struct node *elm)
{
	size_t newsize = 1, newheight = 0;
	if ((RB_LEFT(elm, node_link))) {
		newsize += (RB_LEFT(elm, node_link))->size;
		newheight = MAX((RB_LEFT(elm, node_link))->height, newheight);
	}
	if ((RB_RIGHT(elm, node_link))) {
		newsize += (RB_RIGHT(elm, node_link))->size;
		newheight = MAX((RB_RIGHT(elm, node_link))->height, newheight);
	}
	newheight += 1;
	if (elm->size != newsize || elm->height != newheight) {
		elm->size = newsize;
		elm->height = newheight;
		return 1;
	}
	return 0;
}


void
mix_operations(int *perm, int psize, struct node *nodes, int nsize, int insertions, int reads, int do_reads)
{
	int i;
	struct node *tmp, *ins;
	struct node it;
        assert(psize == nsize);
        assert(insertions + reads <= psize);

	for(i = 0; i < insertions; i++) {
		tmp = &(nodes[i]);
		if (tmp == NULL) err(1, "malloc");
		tmp->size = 1;
		tmp->height = 1;
		tmp->key = perm[i];
                //TDEBUGF("inserting %d", tmp->key);
		if (RB_INSERT(tree, &root, tmp) != NULL)
			errx(1, "RB_INSERT failed");
                //print_tree(&root);
                //int rank = RB_RANK(tree, RB_ROOT(&root));
                //TDEBUGF("rank: %d", rank);
                //if (rank == -2)
                //        errx(1, "rank error");
                //TDEBUGF("%p:%p", (void *)root.root->node_link.child[0], (void *)root.root->node_link.child[1]);
	}
	tmp = &(nodes[insertions]);
	tmp->key = ITER + 5;
	tmp->size = 1;
	tmp->height = 1;
	RB_INSERT(tree, &root, tmp);
	if (do_reads) {
		for (i = 0; i < insertions; i++) {
			it.key = perm[i];
			ins = RB_FIND(tree, &root, &it);
			if ((ins == NULL) || ins->key != it.key)
				errx(1, "RB_FIND failed");
		}
		for (i = insertions; i < insertions + reads; i++) {
			it.key = perm[i];
			ins = RB_NFIND(tree, &root, &it);
			if (ins->key < it.key)
				errx(1, "RB_NFIND failed");
		}
                /*
		for (i = insertions; i < insertions + reads; i++) {
			it.key = perm[i];
			ins = RB_PFIND(tree, &root, &it);
			if (ins->key > it.key)
				errx(1, "RB_PFIND failed");
		}*/
	}
}
