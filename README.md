# Fast-Search Suggestion System

## Overview

A Trie-based (Prefix Tree) fast search and autocomplete engine built in C++, designed to manage and query a civil-construction material lexicon with instant prefix lookup.

## Problem Statement

Managing large material and terminology datasets in civil engineering workflows requires fast exact-match search as well as autocomplete-style suggestions as a user types a partial term. This project implements a Trie-based indexing mechanism to support both operations efficiently, independent of dataset size.

## Features

- Fast word insertion and exact-match retrieval
- **Full autocomplete**: returns every word in the dictionary matching a given prefix (not just a yes/no availability check)
- Trie (Prefix Tree) implementation using a fixed 26-pointer array per node
- Dynamic memory allocation and pointer-based tree construction
- Recursive memory cleanup (destructor) to avoid leaks
- Sample civil-construction material lexicon (cement, concrete, aggregate, bitumen, girder, grout, etc.)

## Technologies Used

- C++
- Trie / Prefix Tree data structure
- Pointers and dynamic memory management
- Recursion (DFS) for suggestion collection

## Project Structure

```
Fast-Search-Dictionary-Trie/
│
├── main.cpp
└── README.md
```

## Example

### Inserted Words (sample lexicon)

```
cement, concrete, concreting, column, conduit,
beam, bridge, brick, bitumen, boulder,
gravel, girder, grout, granite,
steel, sand, slab, sealant,
asphalt, aggregate, admixture
```

### Search & Autocomplete Queries

```
Search 'concrete': Found
Search 'con': Not Found (exists only as a prefix, not a complete word)

Autocomplete suggestions for 'con': concrete, concreting, conduit
Autocomplete suggestions for 'b': beam, bitumen, boulder, brick, bridge
Autocomplete suggestions for 'gr': granite, gravel, grout
Autocomplete suggestions for 'xyz': None
```

## Time & Space Complexity

| Operation            | Time Complexity | Notes |
| --------------------- | ---------------- | ----- |
| Insert                | O(L)             | L = length of word |
| Exact Search          | O(L)             | Independent of dictionary size |
| Prefix Check          | O(L)             | |
| Autocomplete (getSuggestions) | O(P + K) | P = prefix length, K = total characters across matches |

**Space:** O(26 × N × L) worst case, since every node reserves a fixed 26-pointer array regardless of actual branching. A hashmap-per-node design would trade a small time cost for lower memory use on sparse branches — noted as a possible optimization.

## Design Notes / Trade-offs

- A Trie was chosen over a hash map because hash maps cannot efficiently answer prefix queries ("all words starting with X") — Tries support this natively by sharing prefix paths.
- The fixed-size array (26 children per node) assumes lowercase English letters only; extending to mixed case, digits, or Unicode would require switching to a map-based child structure.
- Word deletion is not yet implemented; a full implementation would unmark `isEndOfWord` and optionally prune now-empty branches.

## Future Enhancements

- Frequency-based ranking of suggestions (store a hit-count per word, return the most-searched matches first)
- Case-insensitive search
- Word deletion support
- File-based dictionary loading for larger, real-world material datasets

## Learning Outcomes

- Trie (Prefix Tree) implementation and its trade-offs vs. hash maps/BSTs
- Efficient prefix-based string searching
- Pointer manipulation, dynamic memory allocation, and manual memory cleanup
- Time/space complexity analysis of tree-based data structures

## Author

Aniket Raval
B.Tech Civil Engineering, IIT Madras
