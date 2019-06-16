#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "timer.h"
#include "aligned_ptr.hpp"

#define ROUTINE_ALIGNMENT CACHELINE_SIZE

#if _LP64 || __LP64__
#define FMT_ULONG "%lu"
#define FMT_XLONG "%lx"
#else
#define FMT_ULONG "%u"
#define FMT_XLONG "%x"
#endif

typedef float searchitem_t;

static const char arg_space_size[] = "space_size";
static const char arg_alt[] = "alt";

static const size_t log2_lead_in = 4; // number of top-level bsearch iterations bypassed during binned bsearch
static const size_t lead_in = 1 << log2_lead_in;
static const size_t log2_subsize = 4; // depth of the individual tree in the Van Emde Boas forest
static const size_t alignment = 1 << 12; // search space address alignment

size_t found[1];
size_t obfuscator;

// Some of the algorithms below compute log2 of powers of two (POT). A straightforward way to do that (which is
// also recognized by clang and optimised to native ops where available) is to count the set bits in a POT - 1
// (see bitcount() below). Another way is to count the least-significant zero bits in the POT value.  Many ISAs
// have efficient implementations of the latter (i.e. bfs/ctz). More importantly, gcc has a builtin routine that
// utilizes such ISA features - __builtin_ffs(). Here we provide sample implementaitons of ffs() - bitscan(),
// for two ISAs: amd64 and arm64.

#if !defined(__GNUC__)
static int32_t bitscan(
	int32_t x)
{
    if (0 == x)
        return 0;

#if __x86_64__ || __x86_64 || __amd64__ || __amd64
    asm volatile (
        "bsfl %0, %0" : "=r" (x) : "0" (x) : );

#elif __aarch64__
    asm volatile (
        "rbit %w0, %w0\n\t"
        "clz %w0, %w0" : "=r" (x) : "0" (x) : );

#endif
    return x + 1;
}

static int64_t bitscan(
	int64_t x)
{
    if (0 == x)
        return 0;

#if __x86_64__ || __x86_64 || __amd64__ || __amd64
    asm volatile (
        "bsfq %0, %0" : "=r" (x) : "0" (x) : );

#elif __aarch64__
    asm volatile (
        "rbit %0, %0\n\t"
        "clz %0, %0" : "=r" (x) : "0" (x) : );

#endif
    return x + 1;
}

#else
static int32_t bitscan(
	int32_t x)
{
	return __builtin_ffs(x);
}

static int64_t bitscan(
	int64_t x)
{
#if __LP64__ || __LP64
	return __builtin_ffsl(x);

#else
	return __builtin_ffsll(x);

#endif
}

#endif

static size_t bitcount(
	size_t n)
{
	size_t count = 0;

	for (; 0 != n; ++count)
		n &= n - 1;

	return count;
}

#if __clang_major__ > 3 || __clang_major__ == 3 && __clang_minor__ >= 6
#define assume(cond) __builtin_assume(cond)
#else
#define assume(cond) ({ if (!(cond)) __builtin_unreachable(); })
#endif

static size_t log2_from_pot(
	size_t n)
{
	assert(0 != n);
	assert(0 == (n & n - 1));

	assume(0 != n);

	return bitscan(int64_t(n)) - 1;
}

static size_t log2_ceil(
	size_t n)
{
	if (0 == n)
		return 1;

	const size_t max_pot = size_t(-1) ^ size_t(-1) >> 1;

	if (n > max_pot)
		return size_t(-1);

	if (0 == (n & n - 1))
		return n;

	return max_pot >> __builtin_clzl(n) - 1;
}

#include "bsearch.hpp"

static size_t lsearch_standard(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t lsearch_standard(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::lsearch_standard(space, size, key);
}

static size_t lsearch_binned(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t lsearch_binned(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::lsearch_binned< lead_in >(space, size, key);
}

static size_t bsearch_standard(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t bsearch_standard(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::bsearch_standard(space, size, key);
}

static size_t bsearch_binned(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t bsearch_binned(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::bsearch_binned< lead_in >(space, size, key);
}

static size_t bsearch_breadth(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t bsearch_breadth(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::bsearch_breadth(space, size, key);
}

static size_t bsearch_veb(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t bsearch_veb(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::bsearch_veb< log2_subsize >(space, size, key);
}

static size_t bsearch_veb_iter(
	const searchitem_t* const,
	const size_t,
	const searchitem_t) __attribute__ ((aligned(ROUTINE_ALIGNMENT)));

static size_t bsearch_veb_iter(
	const searchitem_t* const space,
	const size_t size,
	const searchitem_t key)
{
	return search::bsearch_veb_iter< log2_subsize >(space, size, key);
}

static size_t verify_lsearch_standard(
	const size_t space_size,
	searchitem_t* space)
{
	aligned_ptr< searchitem_t, alignment > local_space;

	if (0 == space) {
		local_space.malloc(space_size);
		space = local_space;
	}

	for (size_t i = 0; i < space_size; ++i)
		space[i] = searchitem_t(i);

	fprintf(stderr, "verifying lsearch_standard consistency for size " FMT_ULONG ".. ", space_size);

	bool error = false;

	for (size_t i = 0; i < space_size; ++i) {
		const size_t f = lsearch_standard(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

static size_t verify_lsearch_binned(
	const size_t space_size,
	searchitem_t* space)
{
	aligned_ptr< searchitem_t, alignment > local_space;

	if (0 == space) {
		local_space.malloc(space_size + lead_in);
		space = local_space;
	}

	space[0] = searchitem_t(space_size - 1);

	for (size_t i = 1; i < lead_in; ++i)
		space[i] = searchitem_t((lead_in - i) * space_size / lead_in);

	for (size_t i = 0; i < space_size; ++i)
		space[i + lead_in] = searchitem_t(i);

	fprintf(stderr, "verifying lsearch_binned consistency for size " FMT_ULONG ".. ", space_size);

	bool error = false;

	for (size_t i = 0; i < space_size; ++i) {
		const size_t f = lsearch_binned(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

static size_t verify_bsearch_standard(
	const size_t space_size,
	searchitem_t* space)
{
	aligned_ptr< searchitem_t, alignment > local_space;

	if (0 == space) {
		local_space.malloc(space_size);
		space = local_space;
	}

	for (size_t i = 0; i < space_size; ++i)
		space[i] = searchitem_t(i);

	fprintf(stderr, "verifying bsearch_standard consistency for size " FMT_ULONG ".. ", space_size);

	bool error = false;

	for (size_t i = 0; i < space_size; ++i) {
		const size_t f = bsearch_standard(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

static size_t verify_bsearch_binned(
	const size_t space_size,
	searchitem_t* space)
{
	aligned_ptr< searchitem_t, alignment > local_space;

	if (0 == space) {
		local_space.malloc(space_size + lead_in);
		space = local_space;
	}

	space[0] = searchitem_t(space_size - 1);

	for (size_t i = 1; i < lead_in; ++i)
		space[i] = searchitem_t((lead_in - i) * space_size / lead_in);

	for (size_t i = 0; i < space_size; ++i)
		space[i + lead_in] = searchitem_t(i);

	fprintf(stderr, "verifying bsearch_binned consistency for size " FMT_ULONG ".. ", space_size);

	bool error = false;

	for (size_t i = 0; i < space_size; ++i) {
		const size_t f = bsearch_binned(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

template < typename T >
static const T& min(
	const T& a,
	const T& b)
{
	return a < b ? a : b;
}

static size_t verify_bsearch_breadth(
	const size_t raw_space_size,
	searchitem_t* space)
{
	// we process spaces of size power-of-two minus one
	const size_t space_size = log2_ceil(raw_space_size);
	const size_t log2_size = log2_from_pot(space_size);
	const size_t occupied_space_size = min(raw_space_size, space_size - 1);

	aligned_ptr< searchitem_t, alignment > local_space;

	if (0 == space) {
		local_space.malloc(space_size);
		space = local_space;
	}

	for (size_t walk = 0, i = 0; i < log2_size; ++i)
		for (size_t j = 0; j < size_t(1) << i; ++j, ++walk)
			space[walk] = searchitem_t((space_size >> i + 1) * (j * 2 + 1) - 1);

	fprintf(stderr, "verifying bsearch_breadth consistency for size " FMT_ULONG ".. ", occupied_space_size);

	bool error = false;

	for (size_t i = 0; i < occupied_space_size; ++i) {
		const size_t f = bsearch_breadth(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

static size_t verify_bsearch_veb(
	const size_t raw_space_size,
	searchitem_t* space)
{
	// we process spaces of size power-of-two minus one
	const size_t space_size = log2_ceil(raw_space_size);
	const size_t log2_size = log2_from_pot(space_size);
	const size_t occupied_space_size = min(raw_space_size, space_size - 1);

	if (log2_size % log2_subsize) {
		fprintf(stderr, "verifying bsearch_veb consistency for size " FMT_ULONG ".. skipped\n", space_size);
		return 0;
	}

	aligned_ptr< searchitem_t, alignment > local_space;

	// increase the subtree size to 2 ^ log2_subtree, and account for the extra space by counting the subtrees in the big tree
	const size_t subsize = 1 << log2_subsize;
	const size_t total_size = (space_size - 1) / (subsize - 1) * subsize;

	if (0 == space) {
		local_space.malloc(total_size);
		space = local_space;
	}

	for (size_t walk = 0, f = 0; f < log2_size / log2_subsize; ++f)  // forest-depth iteration
		for (size_t t = 0; t < (1 << f * log2_subsize); ++t, ++walk) // forest-breadth iteration
			for (size_t i = 0; i < log2_subsize; ++i)                // tree-depth iteration
				for (size_t j = 0; j < 1 << i; ++j, ++walk)          // tree-breadth iteration
				{
					space[walk] = searchitem_t(t * (1 << log2_size - f * log2_subsize) +
						(1 << log2_size - f * log2_subsize - (i + 1)) * (j * 2 + 1) - 1);
				}

	fprintf(stderr, "verifying bsearch_veb consistency for size " FMT_ULONG ".. ", occupied_space_size);

	bool error = false;

	for (size_t i = 0; i < occupied_space_size; ++i) {
		const size_t f = bsearch_veb(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

static size_t verify_bsearch_veb_iter(
	const size_t raw_space_size,
	searchitem_t* space)
{
	// we process spaces of size power-of-two minus one
	const size_t space_size = log2_ceil(raw_space_size);
	const size_t log2_size = log2_from_pot(space_size);
	const size_t occupied_space_size = min(raw_space_size, space_size - 1);

	if (log2_size % log2_subsize) {
		fprintf(stderr, "verifying bsearch_veb_iter consistency for size " FMT_ULONG ".. skipped\n", space_size);
		return 0;
	}

	aligned_ptr< searchitem_t, alignment > local_space;

	// increase the subtree size to 2 ^ log2_subtree, and account for the extra space by counting the subtrees in the big tree
	const size_t subsize = 1 << log2_subsize;
	const size_t total_size = (space_size - 1) / (subsize - 1) * subsize;

	if (0 == space) {
		local_space.malloc(total_size);
		space = local_space;
	}

	for (size_t walk = 0, f = 0; f < log2_size / log2_subsize; ++f)  // forest-depth iteration
		for (size_t t = 0; t < (1 << f * log2_subsize); ++t, ++walk) // forest-breadth iteration
			for (size_t i = 0; i < log2_subsize; ++i)                // tree-depth iteration
				for (size_t j = 0; j < 1 << i; ++j, ++walk)          // tree-breadth iteration
				{
					space[walk] = searchitem_t(t * (1 << log2_size - f * log2_subsize) +
						(1 << log2_size - f * log2_subsize - (i + 1)) * (j * 2 + 1) - 1);
				}

	fprintf(stderr, "verifying bsearch_veb_iter consistency for size " FMT_ULONG ".. ", occupied_space_size);

	bool error = false;

	for (size_t i = 0; i < occupied_space_size; ++i) {
		const size_t f = bsearch_veb_iter(space, space_size, searchitem_t(i));

		if (size_t(-1) == f || space[f] != searchitem_t(i)) {
			fprintf(stderr, "\nFAILURE at " FMT_ULONG ", size " FMT_ULONG, i, space_size);
			error = true;
		}
	}

	if (!error)
		fprintf(stderr, "done\n");
	else
		putc('\n', stderr);

	return size_t(error);
}

template < typename SEARCHITEM_T, typename KEY_T >
struct Search
{
	typedef size_t (* search)(const SEARCHITEM_T* const, const size_t, const KEY_T);
	typedef size_t (* verify)(const size_t, SEARCHITEM_T*);
};


static int parse_cli(
	int argc,
	char** argv,
	size_t& rep,
	size_t& space_size,
	Search< searchitem_t, searchitem_t >::search& search,
	Search< searchitem_t, searchitem_t >::verify& verify)
{
	bool rep_done = false;

	for (int i = 1; i < argc; ++i) {
		// use a double for all numeric arguments, so the user can specify big numbers in scientific notation
		double input;

		if (0 == strcmp(argv[i], arg_space_size)) {
			if (argc > i + 1 && 1 == sscanf(argv[++i], "%lf", &input) && 0 < input) {
				if (input > unsigned(-1)) {
					fprintf(stderr, "error: %s should not exceed %u\n",
						arg_space_size, 1U << 24);
					return -1;
				}

				space_size = size_t(input);
				continue;
			}
		}

		if (0 == strcmp(argv[i], arg_alt)) {
			if (argc > i + 1 && 1 == sscanf(argv[++i], "%lf", &input) && 0 <= input && 7 >= input) {
				const size_t alt = size_t(input);
				switch (alt) {
				case 1:
					search = bsearch_binned;
					verify = verify_bsearch_binned;
					break;
				case 2:
					search = bsearch_breadth;
					verify = verify_bsearch_breadth;
					break;
				case 3:
					search = bsearch_veb;
					verify = verify_bsearch_veb;
					break;
				case 4:
					search = bsearch_veb_iter;
					verify = verify_bsearch_veb_iter;
					break;
				case 5:
					search = lsearch_standard;
					verify = verify_lsearch_standard;
					break;
				case 6:
					search = lsearch_binned;
					verify = verify_lsearch_binned;
					break;
				}
				continue;
			}
			rep_done = true;
		}

		if (!rep_done && 1 == sscanf(argv[i], "%lf", &input) && 0 < input) {
			rep = size_t(input);
			rep_done = true;
			continue;
		}

		fprintf(stderr, "usage: %s [%s <unsigned>] [%s <unsigned>] [<sample_size>]\n"
			"\talt 0: standard binary search (default)\n"
			"\talt 1: binned binary search\n"
			"\talt 2: breadth-first layout binary search\n"
			"\talt 3: Van Emde Boas (VEB) layout binary search, recursive version\n"
			"\talt 4: VEB layout binary search, iterative version\n"
			"\talt 5: standard linear search\n"
			"\talt 6: binned linear search\n",
			argv[0], arg_space_size, arg_alt);

		return -1;
	}

	return 0;
}


int main(
	int argc,
	char** argv)
{
	size_t rep = 1e7;
	size_t space_size = 2e3;

	Search< searchitem_t, searchitem_t >::search search = bsearch_standard;
	Search< searchitem_t, searchitem_t >::verify verify = verify_bsearch_standard;

	const int cli_res = parse_cli(
		argc, argv,
		rep, space_size, search, verify);

	if (0 != cli_res)
		return cli_res;

	printf("generating search space..\n");

	const uint64_t s0 = timer_ns();

	// generate 'search space' - a sorted array from 0 to space_size - 1
	aligned_ptr< searchitem_t, alignment > space;

	if (search == bsearch_binned ||
		search == lsearch_binned) {

		if (64 > space_size) {
			fprintf(stderr, "error: binned %s requires a minimum space size of 64\n", search == bsearch_binned ? "bsearch" : "lsearch");
			return -1;
		}

		space_size -= 1; // drop one to even ground with BFS and VEB
		space.malloc(space_size + lead_in);

		printf("verifying binned %s consistency for " FMT_ULONG " bins..\n", search == bsearch_binned ? "bsearch" : "lsearch", lead_in);
	}
	else
	if (search == bsearch_breadth) {
		if (space_size & space_size - 1) {
			fprintf(stderr, "error: breadth-first requires a power-of-two space size\n");
			return -1;
		}

		space.malloc(space_size);

		printf("verifying breadth-first bsearch consistency..\n");
	}
	else
	if (search == bsearch_veb ||
		search == bsearch_veb_iter) {

		if (space_size & space_size - 1) {
			fprintf(stderr, "error: Van Emde Boas requires a power-of-two space size\n");
			return -1;
		}

		const size_t log2_size = log2_from_pot(space_size);

		if (log2_size % log2_subsize) {
			fprintf(stderr, "error: Van Emde Boas requires that the log2 of the space size is a multiple of " FMT_ULONG "\n", log2_subsize);
			return -1;
		}

		// increase the subtree size to 2 ^ log2_subtree, and account for the extra space by counting the subtrees in the big tree
		const size_t subsize = 1 << log2_subsize;
		const size_t total_size = (space_size - 1) / (subsize - 1) * subsize;

		space.malloc(total_size);

		printf("verifying Van Emde Boas bsearch consistency for " FMT_ULONG "-deep subtrees..\n", log2_subsize);
	}
	else {
		space_size -= 1; // drop one to even ground with BFS and VEB
		space.malloc(space_size);

		printf("verifying standard %s consistency..\n", search == lsearch_standard ? "lsearch" : "bsearch");
	}

	// generate 'search sample' - a random array of type searchitem_t and size rep
	const aligned_ptr< searchitem_t, alignment > sample(rep);

	for (size_t i = 0; i < rep; ++i)
		sample[i] = searchitem_t(rand() % unsigned(space_size));

	const size_t verify_small = 64;
	const size_t verify_large = 2048;

	for (size_t i = verify_small; i <= verify_large; ++i)
		if (verify(i, 0))
			return -1;

	if (verify(space_size, space))
		return -1;

	const uint64_t ds = timer_ns() - s0;

	if (ds) {
		const double sec = double(ds) * 1e-9;
		printf("elapsed time: %f\n", sec);
	}

	printf("searching..\n");

	const uint64_t t0 = timer_ns();

	for (size_t i = 0; i < rep; ++i)
		found[i * obfuscator] = search(space, space_size, sample[i]);

	const uint64_t dt = timer_ns() - t0;

	if (dt) {
		const double sec = double(dt) * 1e-9;
		printf("elapsed time: %f (" FMT_ULONG " repetitions over a space of " FMT_ULONG ")\n", sec, rep, space_size);
		printf("average searches/s: %f\n", rep / sec);
	}

	return 0;
}
