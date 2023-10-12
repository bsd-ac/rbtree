#include <sys/time.h>

#include <assert.h>
#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include "rbtree.h"

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

//#define RBT_SMALL
//#define RBT_TEST_RANK
//#define RBT_TEST_DIAGNOSTIC
//#define _RBT_DIAGNOSTIC


#define TDEBUGF(fmt, ...)	fprintf(stderr, "%s:%d:%s(): " fmt "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__)


#ifdef __OpenBSD__
#define SEED_RANDOM srandom_deterministic
#else
#define SEED_RANDOM srandom
#endif

int ITER=150000;
int RANK_TEST_ITERATIONS=10000;

#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* declarations */
struct node;
static int compare(const void *, const void *);
static void mix_operations(int *, int, struct node *, int, int, int, int);

static int tree_augment(struct rb_tree *, void *);

//static void print_helper(const struct node *, int);
static void print_tree(const struct node *);

/* definitions */
struct node {
	struct rb_entry	 node_link;
	int		 key;
	size_t		 height;
	size_t		 size;
};

int
get_key(void *node) {
	return ((struct node *)node)->key;
}

struct rb_tree root;
struct rb_type options;

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

	rb_init(&root);
	options.t_compare = &compare;
	options.t_augment = &tree_augment;
	options.t_offset = offsetof(struct node, node_link);
	root.options = &options;

	TDEBUGF("starting random insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done random insertions in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = rb_root(&root);
	assert(ITER + 1 == ins->size);

	TDEBUGF("getting min");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = rb_min(&root);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done getting min in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
	assert(0 == ins->key);

	TDEBUGF("getting max");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = rb_max(&root);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done getting max in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
	assert(ITER + 5 == ins->key);

	ins = rb_root(&root);
        TDEBUGF("getting root");
        assert(rb_remove(&root, ins) == ins);

	assert(ITER == ((struct node *)rb_root(&root))->size);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER; i++) {
		tmp = rb_root(&root);
		assert(NULL != tmp);
		assert(rb_remove(&root, tmp) == tmp);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			assert(-2 != rank);
                        print_tree(&root);
		}
#endif

		if (!(rb_empty(&root)) && ((struct node *)rb_root(&root))->size != ITER - 1 - i)
			errx(1, "rb_remove size error");

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
		tmp = rb_root(&root);
		assert(NULL != tmp);
		assert(rb_remove(&root, tmp) == tmp);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
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
		ins = rb_find(&root, tmp);
		assert(NULL != tmp);
		assert(rb_remove(&root, ins) == ins);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = rb_root(&root);
	if (ins == NULL)
		errx(1, "rb_root error");
	if (rb_remove(&root, ins) != ins)
		errx(1, "rb_remove failed");

	//print_tree(&root);

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);


	//print_tree(&root);

	TDEBUGF("doing find and remove in random order");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i < ITER; i++) {
//		if (i % RANK_TEST_ITERATIONS == 0)
//			TDEBUGF("removal %d: %d", i, perm[i]);
		//TDEBUGF("rank = %d", rb_rank(&root));
		//print_tree(&root);
		tmp->key = perm[i];
		ins = rb_find(&root, tmp);
		if (ins == NULL) {
			errx(1, "rb_find %d failed: %d", i, perm[i]);
		}
		if (rb_remove(&root, ins) == NULL)
			errx(1, "rb_remove failed: %d", i);
//		if (i % RANK_TEST_ITERATIONS == 0) {
		//TDEBUGF("======");
		//print_tree(&root);
		//TDEBUGF("======");
//			rank = rb_rank(&root);
//			if (rank == -2)
//				errx(1, "rank error");
//		}
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = rb_root(&root);
	if (ins == NULL)
		errx(1, "rb_root error");
	if (rb_remove(&root, ins) != ins)
		errx(1, "rb_remove failed");

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
		ins = rb_nfind(&root, tmp);
		if (ins == NULL)
			errx(1, "rb_nfind failed");
		if (rb_remove(&root, ins) == NULL)
			errx(1, "rb_remove failed: %d", i);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

#ifdef rb_next
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree with rb_next");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        tmp = rb_min(&root);
        assert(tmp != NULL);
        assert(tmp->key == 0);
        for(i = 1; i < ITER; i++) {
                tmp = rb_next(&root, tmp);
                assert(tmp != NULL);
                assert(tmp->key == i);
        }
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree with rb_prev");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        tmp = rb_max(&root);
        assert(tmp != NULL);
        assert(tmp->key == ITER + 5);
        for(i = 0; i < ITER; i++) {
                tmp = rb_prev(&root, tmp);
                assert(tmp != NULL);
                assert(tmp->key == ITER - 1 - i);
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER + 1; i++) {
		tmp = rb_root(&root);
		assert(NULL != tmp);
		assert(rb_remove(&root, tmp) == tmp);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef rb_pfind
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
		ins = rb_pfind(&root, tmp);
		if (ins == NULL)
			errx(1, "rb_pfind failed");
		if (rb_remove(&root, ins) == NULL)
			errx(1, "rb_remove failed: %d", i);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
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

#ifdef rb_findc
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
		ins = rb_findc(&root, tmp);
		if (ins == NULL)
			errx(1, "rb_findc failed");
		if (rb_removec(&root, ins) == NULL)
			errx(1, "rb_removec failed: %d", i);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	free(tmp);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done removals in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

	ins = rb_root(&root);
	if (ins == NULL)
		errx(1, "rb_root error");
	if (rb_remove(&root, ins) != ins)
		errx(1, "rb_remove failed");
#endif

#ifdef rb_nfindC
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
		ins = rb_nfindC(&root, tmp);
		if (ins == NULL)
			errx(1, "rb_nfindC failed");
		if (rb_removec(&root, ins) == NULL)
			errx(1, "rb_removec failed: %d", i);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
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

#ifdef rb_pfindC
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
		ins = rb_pfindC(&root, tmp);
		if (ins == NULL)
			errx(1, "rb_pfindC failed");
		if (rb_removec(&root, ins) == NULL)
			errx(1, "rb_removec failed: %d", i);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
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

	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

#ifdef RBT_FOREACH
        TDEBUGF("iterating over tree with RBT_FOREACH");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        i = 0;
        RBT_FOREACH(ins, tree, &root) {
                if (i < ITER)
                        assert(ins->key == i);
                else
                        assert(ins->key == ITER + 5);
                i++;
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef RBT_FOREACH_REVERSE
        TDEBUGF("iterating over tree with RBT_FOREACH_REVERSE");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        i = ITER + 5;
        RBT_FOREACH_REVERSE(ins, tree, &root) {
                assert(ins->key == i);
                if (i > ITER)
                        i = ITER - 1;
                else
                        i--;
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

	TDEBUGF("doing root removals");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER + 1; i++) {
		tmp = rb_root(&root);
		assert(NULL != tmp);
		assert(rb_remove(&root, tmp) == tmp);
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done root removals in: %llu.%09llu s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

#ifdef RBT_FOREACH_SAFE
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree and clearing with RBT_FOREACH_SAFE");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        i = 0;
        RBT_FOREACH_SAFE(ins, tree, &root, tmp) {
                if (i < ITER)
                        assert(ins->key == i);
                else
                        assert(ins->key == ITER + 5);
                i++;
		assert(rb_remove(&root, ins) == ins);
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef RBT_FOREACH_REVERSE_SAFE
	TDEBUGF("starting sequential insertions");
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
	clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
	TDEBUGF("done sequential insertions in: %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree and clearing with RBT_FOREACH_REVERSE_SAFE");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        i = ITER + 5;
        RBT_FOREACH_REVERSE_SAFE(ins, tree, &root, tmp) {
                assert(ins->key == i);
                if (i > ITER)
                        i = ITER - 1;
                else
                        i--;
                assert(rb_remove(&root, ins) == ins);
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef rb_insert_next
        TDEBUGF("starting sequential insertions using INSERT_NEXT");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        tmp = &(nodes[0]);
        tmp->size = 1;
        tmp->height = 1;
        tmp->key = 0;
        if (rb_insert(&root, tmp) != NULL)
                errx(1, "rb_insert failed");
        ins = tmp;
	for(i = 1; i < ITER; i++) {
		tmp = &(nodes[i]);
		tmp->size = 1;
		tmp->height = 1;
		tmp->key = i;
		if (rb_insert_next(&root, ins, tmp) != NULL)
			errx(1, "rb_insert failed");
                ins = tmp;
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done insertions in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree and clearing with RBT_FOREACH_REVERSE_SAFE");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        RBT_FOREACH_REVERSE_SAFE(ins, tree, &root, tmp) {
                assert(rb_remove(&root, ins) == ins);
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
#endif

#ifdef rb_insert_prev
        TDEBUGF("starting sequential insertions using INSERT_PREV");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        tmp = &(nodes[ITER]);
        tmp->size = 1;
        tmp->height = 1;
        tmp->key = ITER;
        if (rb_insert(&root, tmp) != NULL)
                errx(1, "rb_insert failed");
        ins = tmp;
	for(i = ITER - 1; i >= 0; i--) {
		tmp = &(nodes[i]);
		tmp->size = 1;
		tmp->height = 1;
		tmp->key = i;
		if (rb_insert_prev(&root, ins, tmp) != NULL)
			errx(1, "rb_insert failed");
                ins = tmp;
#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done insertions in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);

        TDEBUGF("iterating over tree and clearing with RBT_FOREACH_REVERSE_SAFE");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        RBT_FOREACH_REVERSE_SAFE(ins, tree, &root, tmp) {
                assert(rb_remove(&root, ins) == ins);
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        timespecsub(&end, &start, &diff);
        TDEBUGF("done iterations in %lld.%09ld s", (unsigned long long)diff.tv_sec, (unsigned long long)diff.tv_nsec);
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
		tmp = rb_root(&root);
		if (tmp == NULL)
			errx(1, "rb_root error");
		if (rb_remove(&root, tmp) != tmp)
			errx(1, "rb_remove error");
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
		tmp = rb_root(&root);
		if (tmp == NULL)
			errx(1, "rb_root error");
		if (rb_remove(&root, tmp) != tmp)
			errx(1, "rb_remove error");
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
		tmp = rb_root(&root);
		if (tmp == NULL)
			errx(1, "rb_root error");
		if (rb_remove(&root, tmp) != tmp)
			errx(1, "rb_remove error");
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
		tmp = rb_root(&root);
		if (tmp == NULL)
			errx(1, "rb_root error");
		if (rb_remove(&root, tmp) != tmp)
			errx(1, "rb_remove error");
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
		tmp = rb_root(&root);
		if (tmp == NULL)
			errx(1, "rb_root error");
		if (rb_remove(&root, tmp) != tmp)
			errx(1, "rb_remove error");
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
compare(const void *a, const void *b)
{
	struct node *na = (struct node *)a;
	struct node *nb = (struct node *)b;
	return na->key - nb->key;
}
/**
static void
print_helper(const struct node *n, int indent)
{
	if (rb_right(&root, n))
  		print_helper((struct node *)rb_right(&root, n), indent + 4);
	TDEBUGF("%*s key=%d :: size=%zu :: rank=%d :: l-diff=%d :: r-diff=%d :: addr=%p ", indent, "", n->key, n->size, rb_rank_node(&root, n), rb_rank_diff(&root, n, 0), rb_rank_diff(&root, n, 1), n);
	if (rb_left(&root, n))
  		print_helper((struct node *)rb_left(&root, n), indent + 4);
}
*/
static void
print_tree(const struct node *t)
{
	//if (rb_root(t)) print_helper(rb_root(t), 0);
}

static int
tree_augment(struct rb_tree *rbt, void *velm)
{
	struct node *elm = (struct node *)velm;
	size_t newsize = 1, newheight = 0;
	struct node *l, *r;
	if ((l = (struct node *)rb_left(rbt, elm)) != NULL) {
		newsize += l->size;
		newheight = MAX(l->height, newheight);
	}
	if ((r = (struct node *)rb_right(rbt, elm)) != NULL) {
		newsize += r->size;
		newheight = MAX(r->height, newheight);
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
		if (rb_insert(&root, tmp) != NULL)
			errx(1, "rb_insert failed");
                //print_tree(&root);
                //TDEBUGF("size = %zu", ((struct node *)rb_root(&root))->size);
                assert(((struct node *)rb_root(&root))->size == i + 1);

#ifdef RBT_TEST_RANK
		if (i % RANK_TEST_ITERATIONS == 0) {
			rank = rb_rank(tree, rb_root(&root));
			if (rank == -2)
				errx(1, "rank error");
		}
#endif
	}
	tmp = &(nodes[insertions]);
	tmp->key = ITER + 5;
	tmp->size = 1;
	tmp->height = 1;
	rb_insert(&root, tmp);
	if (do_reads) {
		for (i = 0; i < insertions; i++) {
			it.key = perm[i];
			ins = rb_find(&root, &it);
			if ((ins == NULL) || ins->key != it.key)
				errx(1, "rb_find failed");
		}
		for (i = insertions; i < insertions + reads; i++) {
			it.key = perm[i];
			ins = rb_nfind(&root, &it);
			if (ins->key < it.key)
				errx(1, "rb_nfind failed");
		}
	}
}
