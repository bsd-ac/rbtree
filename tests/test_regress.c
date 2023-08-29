#include <sys/time.h>

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

struct timespec start, end, diff, rstart, rend, rdiff, rtot = {0, 0};
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
#ifndef timespecadd
#define	timespecadd(tsp, usp, vsp)					\
	do {								\
		(vsp)->tv_sec = (tsp)->tv_sec + (usp)->tv_sec;		\
		(vsp)->tv_nsec = (tsp)->tv_nsec + (usp)->tv_nsec;	\
		if ((vsp)->tv_nsec >= 1000000000L) {			\
			(vsp)->tv_sec++;				\
			(vsp)->tv_nsec -= 1000000000L;			\
		}							\
	} while (0)
#endif

//#define RB_SMALL
//#define RB_TEST_RANK
//#define RB_TEST_DIAGNOSTIC
//#define RB_DIAGNOSTIC
#include "tree.h"

#define TDEBUGF(fmt, ...)	fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


#ifdef __OpenBSD__
#define SEED_RANDOM srandom_deterministic
#else
#define SEED_RANDOM srandom
#endif

int ITER=1500000;
int RANK_TEST_ITERATIONS=10000;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* declarations */
struct node;
struct tree;
static int compare(const struct node *, const struct node *);
static void mix_operations(int *, int, struct node *, int, int, int, int);

#ifdef DOAUGMENT
static int tree_augment(struct node *);
#else
#define tree_augment(x) (0)
#endif

#ifdef RB_TEST_DIAGNOSTIC
static void print_helper(const struct node *, int);
static void print_tree(const struct tree *);
#else
#define print_helper(x, y)      do {} while (0)
#define print_tree(x)	   do {} while (0)
#endif

/* definitions */
struct node {
	RB_ENTRY(node)   node_link;
	int		 key;
	size_t		 height;
	size_t		 size;
};

RB_HEAD(tree, node);
struct tree root;

//#undef RB_AUGMENT
//#define RB_AUGMENT(elm) tree_augment(elm)

RB_PROTOTYPE(tree, node, node_link, compare)

RB_GENERATE(tree, node, node_link, compare)

#ifndef RB_RANK
#define RB_RANK(x, y)   0
#endif
#ifndef _RB_GET_RDIFF
#define _RB_GET_RDIFF(x, y, z) 0
#endif

int
main()
{
	struct node *tmp, *ins, *nodes;
	int i, r, rank, *perm, *nums;

	nodes = calloc((ITER + 5), sizeof(struct node));
	perm = calloc(ITER, sizeof(int));
	nums = calloc(ITER, sizeof(int));

	// for determinism
	SEED_RANDOM(4201);

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

#ifdef DOAUGMENT
	ins = RB_ROOT(&root);
	assert(ITER + 1 == ins->size);
#endif

	TDEBUGF("getting min");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MIN(tree, &root);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done getting min in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
	assert(0 == ins->key);

	TDEBUGF("getting max");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB_MAX(tree, &root);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done getting max in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
	assert(ITER + 5 == ins->key);

	ins = RB_ROOT(&root);
        TDEBUGF("getting root");
        assert(RB_REMOVE(tree, &root, ins) == ins);

#ifdef DOAUGMENT
	assert(ITER == (RB_ROOT(&root))->size);
#endif

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER; i++) {
		tmp = RB_ROOT(&root);
		assert(NULL != tmp);
		assert(RB_REMOVE(tree, &root, tmp) == tmp);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			assert(-2 != rank);
                        TDEBUGF("iteration %d, rank %d", i, rank);
                        print_tree(&root);
		}
#endif

#ifdef DOAUGMENT
		if (!(RB_EMPTY(&root)) && (RB_ROOT(&root))->size != ITER - 1 - i)
			errx(1, "RB_REMOVE size error");
#endif

	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER + 1; i++) {
		tmp = RB_ROOT(&root);
		assert(NULL != tmp);
		assert(RB_REMOVE(tree, &root, tmp) == tmp);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing find and remove in sequential order");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER; i++) {
		tmp->key = i;
		ins = RB_FIND(tree, &root, tmp);
		assert(NULL != tmp);
		assert(RB_REMOVE(tree, &root, ins) == ins);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = RB_ROOT(&root);
	if (ins == NULL)
		errx(1, "RB_ROOT error");
	if (RB_REMOVE(tree, &root, ins) != ins)
		errx(1, "RB_REMOVE failed");

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);


	TDEBUGF("doing find and remove in random order");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER; i++) {
		tmp->key = perm[i];
		ins = RB_FIND(tree, &root, tmp);
		if (ins == NULL) {
			errx(1, "RB_FIND %d failed: %d", i, perm[i]);
		}
		if (RB_REMOVE(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVE failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = RB_ROOT(&root);
	if (ins == NULL)
		errx(1, "RB_ROOT error");
	if (RB_REMOVE(tree, &root, ins) != ins)
		errx(1, "RB_REMOVE failed");

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing nfind and remove");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER + 1; i++) {
		tmp->key = i;
		ins = RB_NFIND(tree, &root, tmp);
		if (ins == NULL)
			errx(1, "RB_NFIND failed");
		if (RB_REMOVE(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVE failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

#ifdef RB_PFIND
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing pfind and remove");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER + 1; i++) {
		tmp->key = ITER + 6;
		ins = RB_PFIND(tree, &root, tmp);
		if (ins == NULL)
			errx(1, "RB_PFIND failed");
		if (RB_REMOVE(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVE failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef RB_FINDC
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing findc and removec");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER; i++) {
		tmp->key = i;
		ins = RB_FINDC(tree, &root, tmp);
		if (ins == NULL)
			errx(1, "RB_FINDC failed");
		if (RB_REMOVEC(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVEC failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = RB_ROOT(&root);
	if (ins == NULL)
		errx(1, "RB_ROOT error");
	if (RB_REMOVE(tree, &root, ins) != ins)
		errx(1, "RB_REMOVE failed");
#endif

#ifdef RB_NFINDC
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing nfindc and removec");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER + 1; i++) {
		tmp->key = i;
		ins = RB_NFINDC(tree, &root, tmp);
		if (ins == NULL)
			errx(1, "RB_NFINDC failed");
		if (RB_REMOVEC(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVEC failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef RB_PFINDC
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing pfindc and removec");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER + 1; i++) {
		tmp->key = ITER + 6;
		ins = RB_PFINDC(tree, &root, tmp);
		if (ins == NULL)
			errx(1, "RB_PFINDC failed");
		if (RB_REMOVEC(tree, &root, ins) == NULL)
			errx(1, "RB_REMOVEC failed: %d", i);
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);	
#endif

	TDEBUGF("doing 50%% insertions, 50%% lookups");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 2, ITER / 2, 1);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done operations in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER / 2 + 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing 20%% insertions, 80%% lookups");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 5, 4 * (ITER / 5), 1);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done operations in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER / 5 + 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing 10%% insertions, 90%% lookups");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 10, 9 * (ITER / 10), 1);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done operations in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER / 10 + 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing 5%% insertions, 95%% lookups");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 5 * (ITER / 100), 95 * (ITER / 100), 1);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done operations in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < 5 * (ITER / 100) + 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing 2%% insertions, 98%% lookups");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 2 * (ITER / 100), 98 * (ITER / 100), 1);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done operations in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < 2 * (ITER / 100) + 1; i++) {
		tmp = RB_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB_ROOT error");
		if (RB_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB_REMOVE error");
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

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

#ifdef RB_TEST_DIAGNOSTIC
static void
print_helper(const struct node *n, int indent)
{
	if (RB_RIGHT(n, node_link))
  		print_helper(RB_RIGHT(n, node_link), indent + 4);
	TDEBUGF("%*s key=%d :: size=%zu :: rank=%d :: rdiff %lu:%lu", indent, "", n->key, n->size, RB_RANK(tree, n), _RB_GET_RDIFF(n, _RB_LDIR, node_link), _RB_GET_RDIFF(n, _RB_RDIR, node_link));
	if (RB_LEFT(n, node_link))
  		print_helper(RB_LEFT(n, node_link), indent + 4);
}

static void
print_tree(const struct tree *t)
{
	if (RB_ROOT(t)) print_helper(RB_ROOT(t), 0);
}
#endif

#ifdef DOAUGMENT
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
#endif


void
mix_operations(int *perm, int psize, struct node *nodes, int nsize, int insertions, int reads, int do_reads)
{
	int i, rank;
	struct node *tmp, *ins;
	struct node it;
	assert(psize == nsize);
	assert(insertions + reads <= psize);

	for(i = 0; i < insertions; i++) {
                //TDEBUGF("iteration %d", i);
		tmp = &(nodes[i]);
		if (tmp == NULL) err(1, "malloc");
		tmp->size = 1;
		tmp->height = 1;
		tmp->key = perm[i];
		//TDEBUGF("inserting %d", tmp->key);
		if (RB_INSERT(tree, &root, tmp) != NULL)
			errx(1, "RB_INSERT failed");
#ifdef RB_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = RB_RANK(tree, RB_ROOT(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
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
	}
}
