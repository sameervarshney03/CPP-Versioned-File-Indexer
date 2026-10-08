# Versioned File Indexer

A C++ command-line tool that reads text files in chunks, builds word-frequency indexes, and answers queries across named file versions.

## Features

- Read files incrementally using a configurable buffer of 256–1024 KiB (`KB` in the CLI).
- Extract alphanumeric words and normalize their case.
- Preserve partial words across chunk boundaries and flush the final word at end of file.
- Count a word's occurrences in a named version.
- Display the most frequent words.
- Compare a word's frequency in two separately indexed files.
- Report the configured buffer size and total execution time.

A **version** is a user-supplied name for an in-memory frequency map. This project does not implement revision history or incremental version tracking.

## Implementation status

The following improvements have been successfully implemented and tested:

- [x] Heap-based Top-K with alphabetical tie-breaking.
- [x] Read-only lookups using `find()` and consistent missing-version errors.
- [x] `unique_ptr` ownership for queries and explicit `override` declarations.
- [x] Stronger argument validation and safe character classification.

## Requirements and build

Use GCC/g++ with C++17 support. The original source includes `<bits/stdc++.h>`, which is a GCC-specific convenience header and is not portable to every compiler.

From the directory containing the source:

```bash
g++ -std=c++17 -O2 -Wall -Wextra main.cpp -o analyzer
```

On Windows with a compatible MinGW toolchain, build with `-o analyzer.exe` and invoke `./analyzer.exe` from PowerShell.

## Quick example

Create `v1.txt` with:

```text
Error info error debug error info
```

Create `v2.txt` with:

```text
error info info debug
```

### Count a word

```bash
./analyzer --file v1.txt --version v1 --buffer 256 --query word --word error
```

Expected query result:

```text
Word count = 3
```

### Find the most frequent words

```bash
./analyzer --file v1.txt --version v1 --buffer 256 --query top --top 2
```

Expected query result:

```text
error -> 3
info -> 2
```

### Compare two versions

```bash
./analyzer --file1 v1.txt --version1 v1 --file2 v2.txt --version2 v2 --buffer 256 --query diff --word error
```

Expected query result:

```text
Difference = 2
```

The difference is `count(v1, word) - count(v2, word)`. A negative result means the word occurs more often in the second version. Use distinct version names.

Each successful command also prints the buffer size and elapsed time. Timing varies by machine and input; the outputs above illustrate only the query results.

## Command-line options

| Option | Purpose | Used by |
| --- | --- | --- |
| `--query` | `word`, `top`, or `diff` | All queries |
| `--buffer` | Buffer size in KiB, 256–1024 | All queries |
| `--file` | Input text-file path | `word`, `top` |
| `--version` | Name for the input index | `word`, `top` |
| `--word` | Single alphanumeric search word | `word`, `diff` |
| `--top` | Requested positive number of results | `top` |
| `--file1`, `--file2` | Input paths for comparison | `diff` |
| `--version1`, `--version2` | Distinct names for the two indexes | `diff` |

Quote file paths that contain spaces. The program strictly validates options; missing flags or values will result in a clear error and non-zero exit code.

## How it works

1. `BufferedReader` reads a fixed-size chunk from the file.
2. `Tokenizer` extracts words, converting their characters to lowercase safely.
3. Any unfinished word is carried into the next chunk. For example, `err` followed by `or ` becomes `error`.
4. `VersionedIndex` updates the frequency map for the named version.
5. The selected query reads the resulting index and prints its answer.

| Class or function | Responsibility |
| --- | --- |
| `BufferedReader` | Own the input stream and reading buffer |
| `Tokenizer` | Extract words and preserve incomplete tokens |
| `VersionedIndex` | Store separate frequency maps and provide query operations |
| `Query` | Abstract interface with a virtual destructor |
| `WordQuery` | Execute a word-frequency query |
| `DiffQuery` | Execute a frequency comparison |
| `TopQuery` | Execute a most-frequent-words query |
| `printTopK<T>` | Generic result-printing helper |

The design demonstrates encapsulation, inheritance, runtime polymorphism, function templates, STL containers, exceptions, memory safety (`unique_ptr`), and file I/O.

## Complexity and memory

Let `U` be the number of unique words in a queried version.

- Word and difference queries use average constant-time hash-table lookups, excluding string hashing and normalization costs.
- The size-K heap selects candidates in `O(U log K)` time and sorts the selected results in `O(K log K)` time, for `1 <= K <= U`. The space overhead for the heap is `O(K)`.
- Indexing scans the input and updates word counts. Very long words spanning many chunks are handled safely without overflowing.

**A fixed reading buffer does not imply constant total memory.** Memory also holds the current chunk, temporary tokens, unfinished words, and the vocabulary and counts for every indexed version. The indexes are completely in-memory and are rebuilt upon every execution. Consequently, memory usage inherently grows linearly with the size of the vocabulary of the processed documents. A file with many distinct words can still exhaust available memory.

## Limitations

- Indexes exist only for the current execution. Each command rereads and indexes its input files.
- There is no persistence, parallel indexing, database, or network API.
- Missing-version lookups appropriately throw exceptions now, avoiding unintentional zero-count map insertions.
- Duplicate version names overwrite an existing index, however, identical version names are explicitly prohibited for difference queries.
- Tokenization is byte-oriented and is not a full Unicode text-processing implementation. Character functions process inputs safely as `unsigned char`.
- Frequencies use `int`, so extremely large counts may overflow.

## Acknowledgment

The starting implementation was provided by Velpula Jashwanth as a CS253 file-indexing assignment. Preserve attribution when adapting it, and describe subsequent contributions accurately. This acknowledgment does not grant a software license; confirm permission and applicable terms before publishing reused source code.
