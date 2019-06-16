/// Copyright (c) 2013 Chaos Group
/// This software is provided under the MIT License; see LICENSE file for details.

#if !defined(__bsearch_H__)

#include <cassert>

#ifndef ROUTINE_ALIGNMENT
#define bsearch_ROUTINE_ALIGNMENT CACHELINE_SIZE
#else
#define bsearch_ROUTINE_ALIGNMENT ROUTINE_ALIGNMENT
#endif

namespace search {

inline size_t linear_from_breadth(
	const size_t space_size, // power of two
	const size_t pos,
	const size_t pos_level)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));
	assert(0 <= pos && pos < space_size - 1);
	assert(0 <= pos_level && pos_level < log2_from_pot(space_size));

	return (space_size >> pos_level + 1) * ((pos - (size_t(1) << pos_level) + 1) * 2 + 1) - 1;
}

inline size_t breadth_from_linear(
	const size_t space_size,
	const size_t pos)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));
	assert(0 <= pos && pos < space_size - 1);

	// bottom-up log2 level corresponds to the index of the first unset bit in the position
	const size_t level = bitscan(int64_t(~pos));

	return (size_t(1) << (log2_from_pot(space_size) - level - 1)) + (pos >> (level + 1)) - 1;
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t lsearch_standard(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	size_t i = 0;

	while (i < space_size - 1 && key > space[i])
		++i;

	if (key != space[i])
		i = size_t(-1);

	return i;
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t lnearsearch_standard(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	size_t i = 0;

	while (i < space_size - 1 && key > space[i])
		++i;

	return i;
}

template < size_t LEADIN_SIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t lsearch_binned(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	if (key > space[0])
		return size_t(-1);

	size_t i = 1;

	while (i < LEADIN_SIZE && key < space[i])
		++i;

	const size_t left = (LEADIN_SIZE - i) * space_size / LEADIN_SIZE;
	const size_t right = (LEADIN_SIZE - i + 1) * space_size / LEADIN_SIZE;

	size_t r = lsearch_standard(space + LEADIN_SIZE + left, right - left, key);

	if (r != size_t(-1))
		r += left + LEADIN_SIZE;

	return r;
}

template < size_t LEADIN_SIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t lnearsearch_binned(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	if (key > space[0])
		return space_size - 1;

	size_t i = 1;

	while (i < LEADIN_SIZE && key < space[i])
		++i;

	const size_t left = (LEADIN_SIZE - i) * space_size / LEADIN_SIZE;
	const size_t right = (LEADIN_SIZE - i + 1) * space_size / LEADIN_SIZE;

	return lnearsearch_standard(space + LEADIN_SIZE + left, right - left, key) + left;
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_standard(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	ssize_t left = 0;
	ssize_t right = space_size - 1;

	while (left <= right) {
		const size_t seek_pos = size_t(left + right) / 2;
		const KEY_T k = space[seek_pos];

		if (key < k)
			right = ssize_t(seek_pos - 1);
		else if (key > k)
			left = ssize_t(seek_pos + 1);
		else
			return seek_pos;
	}

	return size_t(-1);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t bnearsearch_standard(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	ssize_t left = 0;
	ssize_t right = space_size - 1;

	while (left <= right) {
		const size_t seek_pos = size_t(left + right) / 2;
		const KEY_T k = space[seek_pos];

		if (key < k)
			right = ssize_t(seek_pos - 1);
		else if (key > k)
			left = ssize_t(seek_pos + 1);
		else
			return seek_pos;
	}

	return right == -1 ? 0 : right;
}

template < size_t LEADIN_SIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_binned(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	if (key > space[0])
		return size_t(-1);

	size_t i = 1;

	while (i < LEADIN_SIZE && key < space[i])
		++i;

	const size_t left = (LEADIN_SIZE - i) * space_size / LEADIN_SIZE;
	const size_t right = (LEADIN_SIZE - i + 1) * space_size / LEADIN_SIZE;

	size_t r = bsearch_standard(space + LEADIN_SIZE + left, right - left, key);

	if (r != size_t(-1))
		r += left + LEADIN_SIZE;

	return r;
}

template < size_t LEADIN_SIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t bnearsearch_binned(
	const KEY_T* const leadin,
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	if (key > leadin[0])
		return space_size - 1;

	size_t i = 1;

	while (i < LEADIN_SIZE && key < leadin[i])
		++i;

	const size_t left = (LEADIN_SIZE - i) * space_size / LEADIN_SIZE;
	const size_t right = (LEADIN_SIZE - i + 1) * space_size / LEADIN_SIZE;

	return bnearsearch_standard(space + left, right - left, key) + left;
}

template < typename SEARCHITEM_T, typename KEY_T >
size_t bsearch_breadth_first(
	const SEARCHITEM_T* const,
	const KEY_T,
	const size_t,
	const size_t,
	const size_t) __attribute__ ((aligned(bsearch_ROUTINE_ALIGNMENT)));

template < typename SEARCHITEM_T, typename KEY_T >
size_t bsearch_breadth_first(
	const SEARCHITEM_T* const space,
	const KEY_T key,
	const size_t level,
	const size_t level_pos,
	const size_t num_level)
{
	if (level == num_level)
		return size_t(-1);

	const size_t level_start = (size_t(1) << level) - 1;
	const size_t seek_pos = level_start + level_pos;
	const KEY_T k = space[seek_pos];

	if (key == k)
		return seek_pos;

	const size_t inc = key > k ? 1 : 0;

	return bsearch_breadth_first(space, key, level + 1, level_pos * 2 + inc, num_level);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_breadth(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));

	return bsearch_breadth_first(space, key, 0, 0, log2_from_pot(space_size));
}

template < typename SEARCHITEM_T, typename KEY_T >
inline void bnearsearch_breadth_first(
	const SEARCHITEM_T* const space,
	const KEY_T key,
	const size_t level,
	const size_t level_pos,
	const size_t num_level,
	size_t& min)
{
	if (level == num_level)
		return;

	const size_t level_start = (size_t(1) << level) - 1;
	const size_t seek_pos = level_start + level_pos;
	const KEY_T k = space[seek_pos];

	if (key == k) {
		min = seek_pos;
		return;
	}

	size_t inc = 0;

	if (key > k) {
		inc = 1;
		min = seek_pos;
	}

	bnearsearch_breadth_first(space, key, level + 1, level_pos * 2 + inc, num_level, min);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline void bnearsearch_breadth(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key,
	size_t& min)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));

	min = (space_size - 1) / 2;

	bnearsearch_breadth_first(space, key, 0, 0, log2_from_pot(space_size), min);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline void bnearsearch_breadth_first(
	const SEARCHITEM_T* const space,
	const KEY_T key,
	const size_t level,
	const size_t level_pos,
	const size_t num_level,
	size_t& min,
	size_t& min_level)
{
	if (level == num_level)
		return;

	const size_t level_start = (size_t(1) << level) - 1;
	const size_t seek_pos = level_start + level_pos;
	const KEY_T k = space[seek_pos];

	if (key == k) {
		min = seek_pos;
		min_level = level;
		return;
	}

	size_t inc = 0;

	if (key > k) {
		inc = 1;
		min = seek_pos;
		min_level = level;
	}

	bnearsearch_breadth_first(space, key, level + 1, level_pos * 2 + inc, num_level, min, min_level);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline void bnearsearch_breadth(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key,
	size_t& min,
	size_t& min_level)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));

	const size_t log2_size = log2_from_pot(space_size);

	min = (space_size - 1) / 2;
	min_level = log2_size - 1;

	bnearsearch_breadth_first(space, key, 0, 0, log2_size, min, min_level);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_van_emde_boas(
	const SEARCHITEM_T* const space,
	const KEY_T key,
	const size_t num_level,
	const size_t macro,
	const size_t macro_pos,
	const size_t macro_base,
	const size_t num_macro)
{
	if (macro == num_macro)
		return size_t(-1);

	const size_t tree_start = macro_pos + macro_base << num_level;
	size_t level_pos = 0;

	for (size_t level = 0; level < num_level; ++level) {
		const size_t level_start = tree_start + (size_t(1) << level) - 1;
		const size_t seek_pos = level_start + level_pos;
		const KEY_T k = space[seek_pos];

		if (key == k)
			return seek_pos;

		const size_t inc = key > k ? 1 : 0;

		level_pos = level_pos * 2 + inc;
	}

	return bsearch_van_emde_boas(space, key, num_level, macro + 1, (macro_pos << num_level) + level_pos,
		macro_base + (1 << macro * num_level), num_macro);
}

template < size_t LOG2_SUBSIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_veb(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));
	assert(0 == log2_from_pot(space_size) % LOG2_SUBSIZE);

	return bsearch_van_emde_boas(space, key, LOG2_SUBSIZE, 0, 0, 0, log2_from_pot(space_size) / LOG2_SUBSIZE);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_van_emde_boas_iter(
	const SEARCHITEM_T* const space,
	const KEY_T key,
	const size_t num_level,
	const size_t num_macro)
{
	size_t macro_pos = 0;
	size_t macro_base = 0;

	for (size_t macro = 0; macro < num_macro; ++macro) {
		const size_t tree_start = macro_pos + macro_base << num_level;
		size_t level_pos = 0;

		for (size_t level = 0; level < num_level; ++level) {
			const size_t level_start = tree_start + (size_t(1) << level) - 1;
			const size_t seek_pos = level_start + level_pos;
			const KEY_T k = space[seek_pos];

			if (key == k)
				return seek_pos;

			const size_t inc = key > k ? 1 : 0;

			level_pos = level_pos * 2 + inc;
		}

		macro_pos = (macro_pos << num_level) + level_pos;
		macro_base += 1 << macro * num_level;
	}

	return size_t(-1);
}

template < size_t LOG2_SUBSIZE, typename SEARCHITEM_T, typename KEY_T >
inline size_t bsearch_veb_iter(
	const SEARCHITEM_T* const space,
	const size_t space_size,
	const KEY_T key)
{
	assert(2 <= space_size && 0 == (space_size & space_size - 1));
	assert(0 == log2_from_pot(space_size) % LOG2_SUBSIZE);

	return bsearch_van_emde_boas_iter(space, key, LOG2_SUBSIZE, log2_from_pot(space_size) / LOG2_SUBSIZE);
}

template < typename SEARCHITEM_T, typename KEY_T >
inline void prepare_for_binned_search(
	KEY_T* const leadin,
	const size_t len_leadin,
	const SEARCHITEM_T* const space,
	const size_t len_space)
{
	leadin[0] = space[len_space - 1];

	for (size_t i = 1; i < len_leadin; ++i)
		leadin[i] = space[(len_leadin - i) * len_space / len_leadin];
}

template < typename SEARCHITEM_T >
inline size_t prepare_for_breadth_search(
	SEARCHITEM_T* const space_dst,
	const size_t len_dst,
	const SEARCHITEM_T* const space_src,
	const size_t len_src)
{
	if (len_src & len_src - 1)
		return 0;

	if (len_src > len_dst)
		return 0;

	for (size_t walk = 0, i = 0; i < log2_from_pot(len_src); ++i)
		for (size_t j = 0; j < size_t(1) << i; ++j, ++walk)
			space_dst[walk] = space_src[(len_src >> (i + 1)) * (j * 2 + 1) - 1];

	return 1;
}

template < typename SEARCHITEM_T >
inline size_t prepare_for_veb_search(
	SEARCHITEM_T* const space_dst,
	const size_t len_dst,
	const SEARCHITEM_T* const space_src,
	const size_t len_src,
	const size_t subsize)
{
	if (len_src & len_src - 1)
		return 0;

	if (subsize & subsize - 1)
		return 0;

	const size_t log2_size = log2_from_pot(len_src);
	const size_t log2_subsize = log2_from_pot(subsize);

	if (log2_size % log2_subsize)
		return 0;

	// increase the subtree size to 2 ^ log2_subtree; account for the extra space by counting the subtrees in the big tree
	const size_t total_size = (len_src - 1) / (subsize - 1) * subsize;

	if (total_size > len_dst)
		return 0;

	for (size_t f = 0, walk = 0; f < log2_size / log2_subsize; ++f)    // tree depth iteration
		for (size_t t = 0; t < (1 << f * log2_subsize); ++t, ++walk) { // tree breadth iteration
			for (size_t i = 0; i < log2_subsize; ++i)                  // subtree depth iteration
				for (size_t j = 0; j < 1 << i; ++j, ++walk) {          // subtree breadth iteration
					space_dst[walk] = space_src[t * (1 << log2_size - f * log2_subsize) +
						(1 << log2_size - f * log2_subsize - (i + 1)) * (j * 2 + 1) - 1];
				}

			space_dst[walk] = 0.f; // padding to subtree size of 2 ^ log2_subtree
		}

	return 1;
}

} // namespace search

#undef bsearch_ROUTINE_ALIGNMENT

#endif // __bsearch_H__
