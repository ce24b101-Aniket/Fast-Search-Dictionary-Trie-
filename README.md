# Fast-Search Suggestion System

## Overview

Fast-Search Suggestion System is a Trie-based search engine developed in C++. It enables efficient word lookup and autocomplete suggestions by leveraging the Prefix Tree (Trie) data structure.

## Problem Statement

Modern search engines and dictionaries require fast retrieval of words and intelligent autocomplete functionality. This project implements a Trie-based indexing mechanism to support efficient prefix matching and word search operations.

## Features

* Fast word insertion and retrieval
* Prefix-based autocomplete suggestions
* Trie (Prefix Tree) implementation
* Efficient dictionary management
* Dynamic memory allocation using pointers

## Technologies Used

* C++
* Trie Data Structure
* Pointers
* Strings
* Dynamic Memory Management

## Project Structure

```text
Fast-Search-Suggestion-System/
│
├── main.cpp
├── README.md
├── sample_words.txt
└── sample_output.txt
```

## Example

### Inserted Words

```text
cement
concrete
column
beam
bridge
brick
```

### Search Queries

```text
Search: concrete
Result: Found

Prefix: con
Suggestions Available
```

## Time Complexity

| Operation     | Complexity |
| ------------- | ---------- |
| Insert        | O(L)       |
| Search        | O(L)       |
| Prefix Search | O(L)       |

where L is the length of the word.

## Future Enhancements

* Ranking suggestions based on frequency
* Spell correction support
* Case-insensitive search
* File-based dictionary loading
* Large-scale dataset integration

## Learning Outcomes

* Trie (Prefix Tree) implementation
* Efficient string searching techniques
* Pointer manipulation and memory management
* Time complexity optimization

## Author

Aniket Raval
B.Tech Civil Engineering, IIT Madras
