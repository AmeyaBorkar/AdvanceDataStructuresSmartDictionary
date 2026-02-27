# Smart Dictionary & Autocomplete Engine

A console-based dictionary application written in C99 that implements and compares three classic tree data structures: **Binary Search Tree (BST)**, **AVL Tree**, and **Threaded Binary Tree (TBT)**. The program supports interactive word lookup, prefix-based autocomplete with usage-ranked suggestions, file-based persistence, and a built-in performance benchmark suite.

---

## Features

- **Three synchronized tree structures** — BST, AVL, and TBT all maintained in parallel; switch between them at runtime to observe behavioral differences
- **Prefix autocomplete** — finds top-K suggestions ranked by corpus frequency score plus personalized usage history
- **Session persistence** — word additions, deletions, and selection counts survive restarts via `custom_words.txt`
- **File loader** — reads pipe-delimited dictionary files in multiple formats (1-field through 5-field)
- **Performance benchmark** — compares insertion time, tree height, search speed, and traversal speed across all three structures at three dataset sizes (500 / 2 000 / 5 000 words)
- **Zero-warning build** — compiles cleanly under `-Wall -Wextra -Wpedantic -std=c99 -g`
- **90 000+ word dictionary** — pre-processed from the [kaikki.org](https://kaikki.org) English dictionary

---

## Architecture

```
┌──────────────────────────────────────────────────────┐
│                     main.c                           │
│         (console menu & application state)           │
└───────────┬────────────┬────────────┬────────────────┘
            │            │            │
       ┌────▼───┐   ┌────▼───┐  ┌────▼───┐
       │ bst.c  │   │ avl.c  │  │ tbt.c  │
       │  BST   │   │  AVL   │  │  TBT   │
       └────────┘   └────────┘  └────────┘
            │            │            │
       ┌────▼────────────▼────────────▼────┐
       │          dictionary.c             │
       │       (WordRecord data model)     │
       └──────────────┬────────────────────┘
                      │
        ┌─────────────┼──────────────┐
   ┌────▼────┐  ┌─────▼─────┐  ┌────▼─────┐
   │loader.c │  │autocomp.. │  │benchmark │
   │ (I/O)  │  │  .c       │  │  .c      │
   └─────────┘  └───────────┘  └──────────┘
```

### Data Structure Comparison

| Property             | BST          | AVL          | TBT (Threaded)         |
|----------------------|-------------|-------------|------------------------|
| Height guarantee     | O(n) worst  | O(log n)    | O(log n) with rebuild  |
| Insert complexity    | O(log n) avg| O(log n)    | O(log n)               |
| Delete complexity    | O(log n) avg| O(log n)    | Rebuild strategy       |
| Inorder traversal    | Recursive   | Recursive   | Iterative (no stack)   |
| Extra memory/node    | None        | Height field| Two thread-flag bits   |

### WordRecord Layout

Each tree node embeds a `WordRecord` struct directly (no extra pointer indirection):

```c
typedef struct {
    char word[64];              // lowercase-normalized key
    char meaning[512];          // definition text
    char part_of_speech[32];    // noun / verb / adj / adv / …
    int  frequency_score;       // corpus frequency (1–100 000)
    int  user_select_count;     // personalization counter
} WordRecord;
```

---

## Prerequisites

| Tool | Version | Notes |
|------|---------|-------|
| GCC  | ≥ 9.0   | Via [MSYS2 MinGW-w64](https://www.msys2.org/) on Windows |
| GNU Make | any | Comes with MSYS2 |
| Python 3 | ≥ 3.8 | Only needed to re-run the preprocessor |

### Windows setup (MSYS2)

```bash
# Install MSYS2 from https://www.msys2.org/, then in its shell:
pacman -S mingw-w64-x86_64-gcc make

# Add to your Windows PATH:
# C:\msys64\mingw64\bin
```

---

## Build

```bash
# Clone the repository
git clone https://github.com/<your-username>/smart-dictionary.git
cd smart-dictionary

# Build (from MSYS2 / Git Bash)
export PATH="/c/msys64/mingw64/bin:$PATH"
make

# Run immediately after building
make run

# Clean build artifacts
make clean

# Rebuild from scratch
make rebuild
```

The compiled binary is `smart_dict.exe`.

---

## Usage

Launch the program from the project root so it can locate the `data/` directory:

```
./smart_dict.exe
```

```
╔══════════════════════════════════════════╗
║  Smart Dictionary & Autocomplete Engine  ║
╚══════════════════════════════════════════╝

  Active tree: AVL

  1. Search word
  2. Insert word
  3. Delete word
  4. Autocomplete
  5. Display all words
  6. Load from file
  7. Switch tree
  8. Benchmark
  9. Save session
  0. About
  Q. Quit
```

### Key interactions

| Menu | Action |
|------|--------|
| **1 – Search** | Enter a word; displays definition, POS, frequency score, and pick count |
| **2 – Insert** | Add a new word with definition and POS tag |
| **3 – Delete** | Remove a word from all three trees |
| **4 – Autocomplete** | Type a prefix; returns top-K suggestions ranked by score |
| **5 – Display all** | Inorder traversal of the active tree (sorted output) |
| **6 – Load file** | Reload from `words.txt`, `words_original.txt`, or enter a custom path |
| **7 – Switch tree** | Toggle the active structure (BST / AVL / TBT) |
| **8 – Benchmark** | Run timed comparison across all three trees |
| **9 – Save** | Persist current words + counts to `data/custom_words.txt` |

### Autocomplete scoring

```
composite_score = frequency_score + 10 × user_select_count
```

Words the user has chosen more often rank higher, creating a personalized experience that improves with use.

---

## Project Structure

```
smart_dictionary/
├── data/
│   ├── words.txt            # 90 000-word pipe-delimited dictionary
│   ├── words_original.txt   # Curated 100-word test set
│   └── word_freq.txt        # Corpus frequency scores (~97 entries)
│
├── main.c                   # Console UI and application orchestration
├── config.h                 # Global constants and file paths
│
├── dictionary.c / .h        # WordRecord struct and utilities
├── utils.c / .h             # String helpers and console I/O
│
├── bst.c / .h               # Unbalanced Binary Search Tree
├── avl.c / .h               # AVL self-balancing BST
├── tbt.c / .h               # Threaded Binary Tree (Knuth header)
│
├── loader.c / .h            # File I/O and multi-format parser
├── autocomplete.c / .h      # Prefix search + ranked suggestions
├── benchmark.c / .h         # Timed performance comparison suite
│
├── preprocess_jsonl.py      # One-time script: JSONL → words.txt
├── Makefile
└── .gitignore
```

---

## Data Files

### `data/words.txt`
Pipe-delimited dictionary with 90 000+ entries:
```
word|part_of_speech|meaning
adhesin|noun|Any of several factors that enable bacteria to adhere...
```
Pre-processed from the [kaikki.org English dictionary](https://kaikki.org/dictionary/English/) using `preprocess_jsonl.py`.

### `data/words_original.txt`
~100 manually curated entries organized by POS category — useful for quick testing.

### `data/word_freq.txt`
CSV frequency scores for a subset of common words:
```
tiger,72
ocean,88
language,92
```

### `data/custom_words.txt` *(generated at runtime)*
5-field format persisting user additions and selection history:
```
word|part_of_speech|meaning|frequency_score|user_select_count
```

---

## Regenerating the Dictionary (optional)

If you want to regenerate `words.txt` from the raw source:

1. Download `kaikki.org-dictionary-English.jsonl` from [kaikki.org](https://kaikki.org/dictionary/English/)
2. Place it in `data/`
3. Run the preprocessor:
   ```bash
   python preprocess_jsonl.py
   ```
   This produces `data/words_processed.txt` (rename to `words.txt` to use it).

---

## Benchmark Sample Output

```
=== Benchmark Results ===

Dataset   Structure   Insert(ms)  Height  Search(ms)  Traverse(ms)
--------  ----------  ----------  ------  ----------  ------------
  500     BST              2         18        1            0
  500     AVL              3          9        1            0
  500     TBT              2          9        1            0
 2000     BST             10         28        4            1
 2000     AVL             12         11        3            1
 2000     TBT             10         11        3            1
 5000     BST             25         37        9            2
 5000     AVL             30         13        7            2
 5000     TBT             23         13        7            1
```

AVL and TBT maintain logarithmic height regardless of insertion order; BST degrades toward O(n) on sorted or nearly-sorted input.

---

## Implementation Notes

- **Single allocation per node** — `WordRecord` is embedded directly in the node struct; no separate heap allocation for strings
- **AVL root capture** — `avl_insert` and `avl_delete` return the new root; callers must always assign the return value
- **TBT delete rebuild** — deletion collects all records, frees all nodes, resets the header, then re-inserts; this keeps the thread logic simple and correct
- **Knuth header/sentinel** — the TBT header node's right pointer points to the true root; its left pointer is a self-reference when the tree is empty
- **Case-insensitive keys** — all words are normalized to lowercase on insert; `str_tolower(dst, src, size)` writes to a separate buffer (never in-place)
- **`input_read_line`** — wraps `fgets` + `str_trim` to avoid the `scanf` newline-leftover bug

---

## License

This project is released for educational use. The dictionary data (`words.txt`) is derived from the [kaikki.org](https://kaikki.org) dataset, which is based on [Wiktionary](https://www.wiktionary.org/) content licensed under [CC BY-SA 3.0](https://creativecommons.org/licenses/by-sa/3.0/).
