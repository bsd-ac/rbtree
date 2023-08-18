#include <sys/time.h>

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#define RB2_SMALL
#include "rbtree.h"

#ifdef __OpenBSD__
#define SEED_RANDOM srandom_deterministic
#else
#define SEED_RANDOM srandom
#endif

#define ITER 20
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
	RB2_ENTRY(node) node_link;
	int             key;
	size_t          height;
	size_t          size;
};

RB2_HEAD(tree, node);

#undef RB2_AUGMENT
#define RB2_AUGMENT(elm) tree_augment(elm)

RB2_PROTOTYPE(tree, node, node_link, compare);

RB2_GENERATE(tree, node, node_link, compare);

int
main(int argc, char **argv)
{
        struct tree root;
	struct node *tmp, *ins, *it, *nodes;
	int i, r, *perm, *nums;
        struct timespec start, end, diff;

        // for determinism
        SEED_RANDOM(42);

	nodes = malloc((ITER + 5) * sizeof(struct node));
	
        printf("generating a 'random' permutation\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
        perm = malloc(sizeof(int) * ITER);
	nums = malloc(sizeof(int) * ITER);

        perm[0] = 0;
	nums[0] = 0;
        for(i = 1; i < ITER; i++) {
		r = random() % i; // arc4random_uniform(i);
                perm[i] = perm[r];
                perm[r] = i;
		nums[i] = i;
        }
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        printf("done generating a 'random' permutation in: %lld.%09ld s\n", end.tv_sec - start.tv_sec, end.tv_nsec - start.tv_nsec);

	RB2_INIT(&root);

        printf("starting random insertions\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done random insertions in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

#ifdef DIAGNOSTIC
	print_tree(RB2_ROOT(&root));
#endif

#ifdef DOAUGMENT
	ins = RB2_ROOT(&root);
	if (ins->size != ITER + 1)
		errx(1, "size does not match");
#endif

	printf("getting min\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB2_MIN(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done getting min in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);
	if (ins->key != 0)
		errx(1, "min does not match");

	printf("getting max\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	ins = RB2_MAX(tree, &root);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done getting max in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);
	if (ins->key != ITER + 5)
		errx(1, "max does not match");

	ins = RB2_ROOT(&root);
	if (RB2_REMOVE(tree, &root, ins) != ins)
		errx(1, "RB2_REMOVE failed");
#ifdef DOAUGMENT
	if ((RB2_ROOT(&root))->size != ITER)
	  errx(1, "RB2_REMOVE initial size error: %zu", (RB2_ROOT(&root))->size);
#endif

	printf("doing root removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	for (i = 0; i < ITER; i++) {
		tmp = RB2_ROOT(&root);
		if (tmp == NULL)
			errx(1, "RB2_ROOT error");
		if (RB2_REMOVE(tree, &root, tmp) != tmp)
			errx(1, "RB2_REMOVE error");
#ifdef DOAUGMENT
		if (!(RB2_EMPTY(&root)) && (RB2_ROOT(&root))->size != ITER - 1 - i)
			errx(1, "RB2_REMOVE size error");
#endif
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done root removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}

        printf("starting sequential insertions\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential insertions in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

        printf("starting sequential insertions\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(nums, ITER, nodes, ITER, ITER, 0, 0);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential insertions in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing find and remove\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	tmp = malloc(sizeof(struct node));
	for(i = 0; i <= ITER; i++) {
		tmp->key = i;
		ins = RB2_FIND(tree, &root, tmp);
		if (ins != NULL)
			if (RB2_REMOVE(tree, &root, ins) != ins)
				errx(1, "RB2_REMOVE failed: %d", i);
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing 50%% insertions, 50%% lookups\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 2, ITER / 2, 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done operations in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing 20%% insertions, 80%% lookups\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 5, 4 * (ITER / 5), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done operations in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing 10%% insertions, 90%% lookups\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, ITER / 10, 9 * (ITER / 10), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done operations in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing 5%% insertions, 95%% lookups\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 5 * (ITER / 100), 95 * (ITER / 100), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done operations in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing 2%% insertions, 98%% lookups\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	mix_operations(perm, ITER, nodes, ITER, 2 * (ITER / 100), 98 * (ITER / 100), 1);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done operations in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

	printf("doing sequential removals\n");
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &start);
	RB2_FOREACH_SAFE(ins, tree, &root, tmp) {
		if(RB2_REMOVE(tree, &root, ins) != ins)
			errx(1, "RB2_REMOVE error");
	}
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
	timespecsub(&end, &start, &diff);
        printf("done sequential removals in: %lld.%09ld s\n", diff.tv_sec, diff.tv_nsec);

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
	if (RB2_LEFT(n, node_link))
  		print_helper(RB2_LEFT(n, node_link), indent + 4);
	printf("%*s key=%d :: size=%zu\n", indent, "", n->key, n->size);
	if (RB2_RIGHT(n, node_link))
  		print_helper(RB2_RIGHT(n, node_link), indent + 4);
}

static void
print_tree(const struct tree *t)
{
	print_helper(RB2_ROOT(t), 0);
}

static int 
tree_augment(struct node *elm)
{
	size_t newsize = 1, newheight = 0;
	if ((RB2_LEFT(elm, node_link))) {
		newsize += (RB2_LEFT(elm, node_link))->size;
		newheight = MAX((RB2_LEFT(elm, node_link))->height, newheight);
	}
	if ((RB2_RIGHT(elm, node_link))) {
		newsize += (RB2_RIGHT(elm, node_link))->size;
		newheight = MAX((RB2_RIGHT(elm, node_link))->height, newheight);
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

	for(i = 0; i < insertions; i++) {
		tmp = &(nodes[i]);
		if (tmp == NULL) err(1, "malloc");
		tmp->size = 1;
		tmp->height = 1;
		tmp->key = perm[i];
		if (RB2_INSERT(tree, &root, tmp) != NULL)
			errx(1, "RB2_INSERT failed");
	}
	tmp = &(nodes[insertions]);
	tmp->key = ITER + 5;
	tmp->size = 1;
	tmp->height = 1;
	RB2_INSERT(tree, &root, tmp);
	if (do_reads) {
		for (i = 0; i < insertions; i++) {
			it.key = perm[i];
			ins = RB2_FIND(tree, &root, &it);
			if ((ins == NULL) || ins->key != it.key)
				errx(1, "RB2_FIND failed");
		}
		for (i = insertions; i < insertions + reads; i++) {
			it.key = perm[i];
			ins = RB2_NFIND(tree, &root, &it);
			if (ins->key < it.key)
				errx(1, "RB2_NFIND failed");
		}
	}
}
