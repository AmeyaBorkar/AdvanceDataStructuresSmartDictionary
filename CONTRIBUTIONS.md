# Contributions & Project History

## Team

| Member | Role |
|--------|------|
| [Ameya Borkar](#ameya-borkar) | Lead & Architect |
| [Aarush Bakshi](#aarush-bakshi) | Data & Utilities |
| [Arnav Gupta](#arnav-gupta) | BST & Benchmarking |
| [Ayush Agnihotri](#ayush-agnihotri) | AVL & Autocomplete |
| [Aditya Chimurkar](#aditya-chimurkar) | Threaded Binary Tree |

---

## Individual Contributions

### Ameya Borkar
**Role: Lead & Architect**

Owned the overall system architecture and was responsible for all integration, final polish, and cross-module correctness. Defined the module contract boundaries so all five contributors could work without blocking each other.

**Files owned:**
- `config.h` — defined every global constant, buffer size, capacity limit, and file path used across the entire codebase; a change here ripples into all nine source files
- `dictionary.c / dictionary.h` — designed and implemented the `WordRecord` struct (fixed-size embedded char arrays for single-allocation nodes), `word_record_init`, `word_record_compare` (case-insensitive), and `word_record_print`
- `main.c` — built the complete console UI: all menu handlers (`menu_search_word`, `menu_insert_word`, `menu_delete_word`, `menu_autocomplete`, `menu_display_all`, `menu_load_from_file`, `menu_switch_tree`, `menu_benchmark`, `menu_about`), global tree state (`g_bst_root`, `g_avl_root`, `g_tbt_header`), active-tree selector, traversal callbacks, startup auto-load logic, and session save/quit flow
- `Makefile` — authored the build system: compiler flags (`-Wall -Wextra -Wpedantic -std=c99 -g`), pattern rule for `.c → .o`, explicit header dependency list for all nine object files, and `all / clean / run / rebuild` targets

**Cross-cutting responsibilities:**
- Designed the three-tree synchronization contract: every insert, delete, and frequency update must touch BST, AVL, and TBT in the same operation
- Performed final integration testing across all 8 phases, verified zero warnings on the full build
- Reviewed all team members' modules for interface correctness before merging into `main.c`
- Resolved the `input_read_line` / `fgets` + `str_trim` pattern that eliminated the `scanf` newline-leftover bug present in early drafts

---

### Aarush Bakshi
**Role: Data & Utilities**

Owned the entire I/O layer — both the runtime file operations and the one-time data-pipeline script that produced the 90 000-word dictionary.

**Files owned:**
- `utils.c / utils.h` — implemented all portable string helpers (`str_tolower`, `str_toupper`, `str_trim` / `str_trim_left` / `str_trim_right`, `str_safe_copy`, `str_starts_with`, `str_starts_with_ci`, `str_is_empty`) and console I/O helpers (`input_flush_stdin`, `input_read_line`, `print_separator`, `print_header`). Critical design decision: `str_tolower(dst, src, size)` is never in-place, preventing a category of subtle aliasing bugs.
- `loader.c / loader.h` — implemented `load_words` (multi-format pipe-delimited parser supporting 1-field through 5-field records, comment/blank-line skipping), `load_frequencies` (CSV word→score reader), and `save_custom_words` (5-field session persistence format). All three functions keep BST, AVL, and TBT in sync on every record loaded.
- `preprocess_jsonl.py` — one-time script that converts the 2.7 GB `kaikki.org-dictionary-English.jsonl` source into the `data/words.txt` pipe-delimited format. Enforces per-field length limits matching `config.h`, deduplicates with POS-priority (noun preferred), and caps output at 90 000 entries.

---

### Arnav Gupta
**Role: BST & Benchmarking**

Implemented the baseline unbalanced tree that all three structures are compared against, and built the entire performance measurement suite.

**Files owned:**
- `bst.c / bst.h` — implemented `bst_new_node`, `bst_insert` (case-insensitive key normalisation), `bst_search`, `bst_delete` (all three cases: leaf, one child, two children via inorder-successor), `bst_inorder` (recursive traversal with callback), `bst_free`, `bst_height`, `bst_count`. Unbalanced — O(log n) average, O(n) worst case — serves as the control group for benchmarking.
- `benchmark.c / benchmark.h` — implemented `benchmark_run_all`: generates deterministic synthetic datasets (`wd00001`–`wd05000`) via Fisher-Yates shuffle with seed 42, then for dataset sizes 500 / 2 000 / 5 000 measures bulk insertion time, resulting tree height, 1 000-repetition search time, and full inorder traversal time across all three tree types, printing a formatted comparison table.

---

### Ayush Agnihotri
**Role: AVL & Autocomplete**

Implemented the self-balancing tree used as the default active structure, and the prefix-search autocomplete engine that sits on top of all three trees.

**Files owned:**
- `avl.c / avl.h` — implemented `avl_new_node`, `avl_insert` (with LL / LR / RR / RL rotations for all four imbalance cases), `avl_delete` (with rebalancing), `avl_search`, `avl_inorder`, `avl_free`, `avl_height`, `avl_balance_factor`, `avl_count`. Critical interface contract: both `avl_insert` and `avl_delete` return the new root — callers must always capture the return value (`g_avl_root = avl_insert(...)`).
- `autocomplete.c / autocomplete.h` — implemented `autocomplete_bst`, `autocomplete_avl`, `autocomplete_tbt` (each collects up to MAX_CANDIDATES=512 prefix matches, sorts by composite score descending, and returns top-K results) and `autocomplete_record_selection` (increments `user_select_count` on the chosen word across all three trees). Composite scoring formula: `frequency_score + 10 × user_select_count`.

---

### Aditya Chimurkar
**Role: Threaded Binary Tree**

Implemented the most algorithmically complex structure in the project — a fully-threaded binary tree using the Knuth header/sentinel pattern that enables stack-free, recursion-free inorder traversal.

**Files owned:**
- `tbt.c / tbt.h` — implemented `tbt_create_header` (sentinel node that self-references when empty), `tbt_new_node` (data node with `lthread`/`rthread` flags), `tbt_insert` (places node and correctly rewires thread pointers for its predecessor and successor), `tbt_search`, `tbt_delete` (rebuild strategy: collect all records → free all nodes → reset header → re-insert, keeping thread logic provably correct), `tbt_inorder` (iterative traversal using thread pointers — no stack, no recursion), `tbt_inorder_successor` (advances via right-thread or leftmost descent), `tbt_free`, `tbt_count`. Thread flag convention: `lthread=0 / rthread=0` means real child pointer; `lthread=1 / rthread=1` means thread pointer to inorder predecessor/successor.

---

## Project Version History

The project was built across 8 sequential phases, grouped here into three released versions.

---

## Version 1.0 — Foundation
*Phases 1 – 3*

### What was built

**Phase 1 — Project setup, data model, utilities, and menu skeleton**

The project began with establishing every constant and interface that later phases would depend on. `config.h` was written first, locking in buffer sizes (`MAX_WORD_LEN=64`, `MAX_MEANING_LEN=512`), capacity limits (`MAX_WORDS=100 000`), and file paths. The `WordRecord` struct was designed with fixed-size `char` arrays instead of `char*` pointers — a deliberate choice that means each tree node requires exactly one `malloc` and one `free` with no risk of dangling string pointers.

`utils.c` was implemented next, providing the string-handling foundation the rest of the project depends on: case conversion, trimming, safe copy, prefix matching, and the `input_read_line` wrapper that eliminated `scanf`-style newline problems before they could cause bugs in later phases.

`main.c` started as a menu skeleton — the numbered option loop was in place with stub handlers, global tree pointers were declared (`g_bst_root`, `g_avl_root`, `g_tbt_header`), and the active-tree selector was wired up. At this stage none of the trees existed yet; the menu printed "not implemented" placeholders.

**Phase 2 — Binary Search Tree**

`bst.c` was the first data structure implemented. The unbalanced BST established the node shape (`BSTNode` embedding `WordRecord` directly), the normalisation contract (all keys stored lowercase), and the traversal callback pattern (`bst_inorder` takes a function pointer) that AVL and TBT would later mirror. All three delete cases (leaf, one child, two children via inorder-successor swap) were implemented and tested against the 15-word hardcoded test set in `main.c`.

At the end of Phase 2 the application could insert, search, delete, and display words in sorted order using the BST. AVL and TBT slots in the menu were still stubs.

**Phase 3 — File loader**

`loader.c` was written to move the application from the hardcoded 15-word test set to real data. The parser was designed to handle multiple pipe-delimited formats simultaneously:

```
word
word|pos
word|pos|meaning
word|pos|meaning|freq|picks
```

This backward-compatible design meant the same loader could read `words_original.txt` (manually curated, 3-field), `words.txt` (preprocessed, 3-field), and `custom_words.txt` (session file, 5-field) without separate code paths.

`load_frequencies` was added to overlay frequency scores from `word_freq.txt` on top of already-inserted records. Because only AVL and TBT existed as stubs, `load_words` inserted into BST only at this stage and was later extended to sync all three trees.

`preprocess_jsonl.py` was also written in this phase to convert the raw 2.7 GB kaikki.org JSONL dump into the pipe-delimited `words.txt`. The script enforces the same length limits as `config.h`, deduplicates by keeping the noun sense when multiple POS entries exist for a word, and stops at 90 000 entries.

### State at v1.0

- Single working tree (BST)
- File loading from `words.txt` and `words_original.txt`
- Frequency score overlay from `word_freq.txt`
- Full console menu with search, insert, delete, display-all
- 90 000-word dictionary file ready
- Build: zero warnings

---

## Version 2.0 — Advanced Data Structures & Autocomplete
*Phases 4 – 6*

### What was built

**Phase 4 — AVL Tree**

`avl.c` added a self-balancing tree that guaranteed O(log n) height regardless of insertion order. The key implementation decision was the return-value interface: `avl_insert` and `avl_delete` both return the (potentially new) root, requiring every call site in `main.c` and `loader.c` to write `g_avl_root = avl_insert(g_avl_root, &rec)`. This is more explicit than a double-pointer approach and makes the rebalancing visible at the call site.

All four rotation cases were implemented (LL, LR, RR, RL). After Phase 4, loading 88 words produced BST height 13 vs AVL height 8 — the first measurable demonstration of balancing.

The loader was updated to keep BST and AVL in sync: every `load_words` call now inserted into both trees.

**Phase 5 — Threaded Binary Tree**

`tbt.c` was the most algorithmically involved phase. The Knuth header/sentinel pattern was chosen: a special header node whose right pointer points to the tree root and whose left pointer is a self-reference when the tree is empty. Every data node carries two extra bit-flags (`lthread`, `rthread`) indicating whether each pointer is a real child (0) or a thread to the inorder predecessor/successor (1).

The payoff is `tbt_inorder`: a fully iterative traversal that follows thread pointers with no stack and no recursion, producing sorted output in O(n) time with O(1) extra memory.

Delete was the hardest operation. Rather than attempting to re-thread around a deleted node (error-prone), a rebuild strategy was used: collect all records from the tree via inorder traversal, free every node, reset the header to the empty state, then re-insert. This trades a small performance cost on delete for provably correct thread pointers after every operation.

The loader was extended again to sync all three trees on every insert.

**Phase 6 — Autocomplete Engine**

`autocomplete.c` built the prefix-search system on top of all three trees. Each tree gets its own traversal function (`autocomplete_bst`, `autocomplete_avl`, `autocomplete_tbt`) that collects every word whose lowercase key starts with the given prefix into a candidate buffer (MAX_CANDIDATES = 512).

Candidates are ranked by composite score:

```
composite_score = frequency_score + 10 × user_select_count
```

The `× 10` multiplier on `user_select_count` means a word selected just once overtakes words with up to 10 points more corpus frequency. This makes the ranking personalise quickly without requiring many interactions.

`autocomplete_record_selection` increments the pick counter on the chosen word across all three trees so the score persists correctly regardless of which tree is active.

### State at v2.0

- All three trees (BST, AVL, TBT) implemented and synchronized
- Runtime tree switching (menu option 7)
- Prefix autocomplete with composite frequency + usage ranking
- Selection tracking updates all three trees simultaneously
- Confirmed BST height 13 vs AVL/TBT height 8 on the 88-word test set

---

## Version 3.0 — Benchmarking, Persistence & Final Release
*Phases 7 – 8*

### What was built

**Phase 7 — Benchmarking Suite**

`benchmark.c` was written to put objective numbers on the height difference observed in Phase 4. The benchmark generates synthetic word keys (`wd00001` through `wd05000`) in deterministic pseudo-random order using a Fisher-Yates shuffle with fixed seed 42, ensuring reproducible results across runs.

For each of three dataset sizes (500, 2 000, 5 000 words) it measures:

| Metric | How measured |
|--------|-------------|
| Bulk insert time | `clock()` before and after all inserts |
| Tree height | Direct call to `bst_height` / `avl_height` / `tbt_count` |
| Search time | 1 000 repeated lookups, averaged |
| Traversal time | Single full inorder pass |

Results are printed as a formatted table to stdout. The benchmark runs entirely in-memory on its own separate tree instances so it does not disturb the loaded dictionary.

**Phase 8 — Integration, Session Persistence, and Final Polish**

Phase 8 connected all existing pieces into the final release state.

*Session persistence* was completed: `save_custom_words` in `loader.c` writes the entire dictionary to `data/custom_words.txt` in 5-field format preserving `frequency_score` and `user_select_count`. On next startup, `main.c` tries `custom_words.txt` first; if absent, it falls back to `words.txt`. This means user personalisation (word additions, deletions, autocomplete selections) survives a restart.

*Auto-load on startup* was implemented: rather than requiring the user to select menu option 6, the application now loads the dictionary automatically during initialisation, so the first thing a user sees is the menu with 90 000 words already in memory.

*Full integration testing* was run: all eight phase features were exercised together, the build was cleaned and rebuilt from scratch, and the final result was zero warnings under `-Wall -Wextra -Wpedantic -std=c99 -g`.

*Makefile* was finalised with explicit header dependency lines for all nine object files, preventing stale-build bugs if any `.h` file is changed.

### State at v3.0 (current)

- 90 005 words loaded from `data/words.txt` on startup
- All three trees in sync at all times
- Autocomplete with personalised composite scoring
- Session persistence via `data/custom_words.txt`
- Full benchmark suite comparing BST / AVL / TBT
- Zero-warning build under strict C99 flags
- All 8 phases complete and verified
