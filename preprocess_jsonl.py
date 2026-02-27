"""
preprocess_jsonl.py
-------------------
Converts kaikki.org-dictionary-English.jsonl into a words_processed.txt file
compatible with the Smart Dictionary loader.

Output format (words.txt / 3-field):
    word|part_of_speech|meaning

Constraints from config.h:
    MAX_WORD_LEN    64   ->  word   <= 63 chars
    MAX_POS_LEN     32   ->  pos    <= 31 chars
    MAX_MEANING_LEN 512  ->  meaning<= 511 chars
    MAX_WORDS    100000  ->  hard cap on entries
"""

import json
import random
import re
import sys

# ── paths ─────────────────────────────────────────────────────────────────────
INPUT_FILE  = r"data\kaikki.org-dictionary-English.jsonl"
OUTPUT_FILE = r"data\words_processed.txt"

# ── limits (mirror config.h) ──────────────────────────────────────────────────
MAX_WORD    = 63
MAX_POS     = 31
MAX_MEANING = 511
MAX_OUTPUT  = 90_000   # leave headroom below MAX_WORDS=100000

# ── POS we want to keep, mapped to full display names ────────────────────────
POS_MAP = {
    "noun":      "noun",
    "verb":      "verb",
    "adj":       "adjective",
    "adv":       "adverb",
    "pron":      "pronoun",
    "prep":      "preposition",
    "conj":      "conjunction",
    "intj":      "interjection",
    "det":       "determiner",
    "num":       "numeral",
    "article":   "article",
}

# POS priority for deduplication (lower index = preferred)
POS_PRIORITY = ["noun", "verb", "adj", "adv", "pron", "prep",
                "conj", "intj", "det", "num", "article"]

# ── glosses to skip (cross-references, inflections, proper names, places) ─────
# Matched case-insensitively against the start of each gloss.
SKIP_PREFIXES = (
    # Inflections – actual kaikki.org phrasing (from corpus analysis)
    "alternative form of",
    "alternative spelling of",
    "alternative letter-case form",
    "alternative capitalization of",
    "present participle and",       # "present participle and gerund of …"
    "third-person singular simple", # "third-person singular simple present …"
    "simple past and",              # "simple past and past participle of …"
    "simple past of",
    "past participle of",
    "plural of",
    "comparative form of",
    "superlative form of",
    "gerund of",
    "inflection of",
    # Archaic / obsolete / non-standard
    "archaic form of",
    "archaic spelling of",
    "obsolete form of",
    "obsolete spelling of",
    "dated form of",
    "dated spelling of",
    "eye dialect spelling of",
    "misspelling of",
    "elongated form of",
    "nonstandard spelling of",
    "nonstandard form of",
    "rare form of",
    "rare spelling of",
    # Abbreviations / short forms
    "abbreviation of",
    "initialism of",
    "acronym of",
    "clipping of",
    "short for",
    # Semantic cross-refs
    "synonym of",
    "antonym of",
    "diminutive of",
    "augmentative of",
    "feminine of",
    "masculine of",
    "genitive of",
    "dative of",
    "nominative of",
    # Proper names (surnames, given names)
    "a surname",
    "a female given",
    "a male given",
    "a placename",
    # Geographic entries – not useful for a word dictionary
    "a village in",
    "a village and",
    "a town in",
    "a city in",
    "a suburb of",
    "a locality in",
    "an unincorporated community",
    "a barangay of",
    "a place in",
)

# ── helpers ───────────────────────────────────────────────────────────────────
_PURE_ALPHA = re.compile(r'^[a-zA-Z]+$')

def is_valid_word(w: str) -> bool:
    return bool(w and _PURE_ALPHA.match(w) and 2 <= len(w) <= MAX_WORD)

def clean_meaning(text: str) -> str:
    """Strip wikitext artifacts and truncate."""
    # Remove anything inside parentheses that looks like a tag/label
    text = re.sub(r'\s*\([^)]{0,40}\)\s*', ' ', text)
    # Remove pipe characters — they are the field delimiter in the output format
    text = text.replace('|', ' ')
    # Collapse whitespace
    text = ' '.join(text.split())
    # Truncate
    if len(text) > MAX_MEANING:
        text = text[:MAX_MEANING - 1].rsplit(' ', 1)[0]
    return text.strip()

_SKIP_LOWER = tuple(p.lower() for p in SKIP_PREFIXES)

def is_skip_gloss(gloss: str) -> bool:
    g = gloss.lower()
    return any(g.startswith(p) for p in _SKIP_LOWER)

def get_best_gloss(senses: list) -> str:
    """Return the first substantive gloss from a list of senses."""
    for sense in senses:
        for gloss in sense.get("glosses", []):
            gloss = gloss.strip()
            if gloss and not is_skip_gloss(gloss):
                return gloss
    return ""

def pos_priority(raw_pos: str) -> int:
    try:
        return POS_PRIORITY.index(raw_pos)
    except ValueError:
        return len(POS_PRIORITY)

# ── main pass ─────────────────────────────────────────────────────────────────
def main():
    print(f"Reading {INPUT_FILE} …", flush=True)

    # dict: word -> (raw_pos, display_pos, meaning)
    # We keep one entry per word; replace if new entry has higher-priority POS.
    best: dict[str, tuple[int, str, str]] = {}

    total_lines = 0
    skipped_pos = 0
    skipped_word = 0
    skipped_gloss = 0

    with open(INPUT_FILE, encoding="utf-8") as f:
        for line in f:
            total_lines += 1
            if total_lines % 100_000 == 0:
                print(f"  … {total_lines:,} lines read, {len(best):,} unique words so far",
                      flush=True)

            try:
                entry = json.loads(line)
            except json.JSONDecodeError:
                continue

            raw_pos = entry.get("pos", "")
            if raw_pos not in POS_MAP:
                skipped_pos += 1
                continue

            word = entry.get("word", "").lower().strip()
            if not is_valid_word(word):
                skipped_word += 1
                continue

            gloss = get_best_gloss(entry.get("senses", []))
            if not gloss:
                skipped_gloss += 1
                continue

            meaning = clean_meaning(gloss)
            if not meaning:
                skipped_gloss += 1
                continue

            display_pos = POS_MAP[raw_pos][:MAX_POS]
            prio = pos_priority(raw_pos)

            if word not in best or prio < best[word][0]:
                best[word] = (prio, display_pos, meaning)

    print(f"\nDone reading {total_lines:,} lines.")
    print(f"  Skipped – bad POS: {skipped_pos:,}, bad word: {skipped_word:,}, "
          f"no gloss: {skipped_gloss:,}")
    print(f"  Unique words collected: {len(best):,}")

    # Selection strategy:
    #   1. Take ALL words of length 2–7 (core vocabulary, ~82k words).
    #   2. For the remaining slots, sample 8+ letter words proportionally by
    #      starting letter (a–z) so that the long-word selection is spread
    #      evenly across the alphabet rather than biased to 'a*' words.
    #   3. Final list is sorted alphabetically for the loader.
    SHORT_MAX_LEN = 7
    short_words = sorted(w for w in best if len(w) <= SHORT_MAX_LEN)
    long_words  = sorted(w for w in best if len(w) >  SHORT_MAX_LEN)

    remaining = MAX_OUTPUT - len(short_words)
    if remaining <= 0:
        print(f"  Capping to {MAX_OUTPUT:,} entries (only short words fit).")
        words_sorted = short_words[:MAX_OUTPUT]
    elif len(long_words) <= remaining:
        words_sorted = short_words + long_words
    else:
        # Group long words by first letter, then stride-sample within each group
        from collections import defaultdict
        by_letter: dict[str, list[str]] = defaultdict(list)
        for w in long_words:
            by_letter[w[0]].append(w)  # already sorted alphabetically

        # Allocate slots proportionally to each letter's count
        total_long = len(long_words)
        sampled: list[str] = []
        allocated = 0
        letters_sorted = sorted(by_letter.keys())
        for i, letter in enumerate(letters_sorted):
            group = by_letter[letter]
            if i == len(letters_sorted) - 1:
                # Last letter: give all remaining slots
                slots = remaining - allocated
            else:
                slots = round(remaining * len(group) / total_long)
            slots = min(slots, len(group))
            if slots <= 0:
                continue
            # Stride-sample within this letter's group
            step = len(group) / slots
            sampled.extend(group[int(j * step)] for j in range(slots))
            allocated += slots

        print(f"  Capping to {MAX_OUTPUT:,} entries: all {len(short_words):,} "
              f"short + {len(sampled):,} sampled long words (spread across a-z).")
        words_sorted = short_words + sampled

    # Shuffle so BST insertion order is random → balanced tree height ~O(log n).
    # (Alphabetical order causes a degenerate right-skewed BST of height N.)
    random.seed(42)
    random.shuffle(words_sorted)

    # Write output
    print(f"\nWriting {OUTPUT_FILE} …", flush=True)
    with open(OUTPUT_FILE, "w", encoding="utf-8") as out:
        out.write("# Smart Dictionary - Preprocessed Word Dataset\n")
        out.write("# Source: kaikki.org-dictionary-English.jsonl\n")
        out.write("# Format: word|part_of_speech|meaning\n")
        out.write("# Lines starting with # are comments and are skipped.\n")
        out.write("#\n")

        written = 0
        for word in words_sorted:
            _, display_pos, meaning = best[word]
            # Final safety truncation (handles edge cases from clean_meaning)
            if len(meaning) > MAX_MEANING:
                meaning = meaning[:MAX_MEANING]
            line_out = f"{word}|{display_pos}|{meaning}\n"
            out.write(line_out)
            written += 1

    print(f"Wrote {written:,} entries to {OUTPUT_FILE}")
    print("\nSample output (first 10 lines of data):")
    with open(OUTPUT_FILE, encoding="utf-8") as f:
        count = 0
        for line in f:
            if line.startswith("#"):
                continue
            print(f"  {line}", end="")
            count += 1
            if count >= 10:
                break

if __name__ == "__main__":
    main()
