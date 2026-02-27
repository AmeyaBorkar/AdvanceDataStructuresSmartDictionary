/* benchmark.h - Performance benchmarking and comparison (Phase 7) */
#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "dictionary.h"
#include "bst.h"
#include "avl.h"
#include "tbt.h"

/*
 * Run the full benchmark suite comparing BST, AVL, and TBT.
 *
 * For each dataset size (500, 2000, 5000 words, pseudo-random insertion order):
 *   - Bulk insertion timing
 *   - Tree height after insertion
 *   - Repeated search timing (1000 lookups)
 *   - Full sorted traversal timing
 *
 * Results are printed as a formatted comparison table to stdout.
 * All trees are built fresh for each trial and freed afterwards.
 * Random seed is fixed (srand=42) for reproducible word order.
 */
void benchmark_run_all(void);

#endif /* BENCHMARK_H */
