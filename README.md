# Memory-Efficient Versioned File Indexer

## Overview
The Memory-Efficient Versioned File Indexer is a comprehensive C++ application designed to process large text files incrementally using a fixed-size buffer. It builds a word-level frequency index over large datasets without loading the entire file into memory, and supports multiple analytical queries across different file versions. The system is designed with strong object-oriented principles, disciplined memory usage, and scalable architecture using standard C++.

## Features

### File Processing
- **Incremental Reading:** Processes files using a fixed-size buffer (256KB–1024KB)
- **Memory Efficiency:** Memory usage is independent of file size
- **Split-Token Handling:** Correctly handles words that span across buffer boundaries

### Versioned Indexing
- **Multiple Versions:** Maintains separate word-frequency indexes for different file versions
- **Version Identification:** Each indexed file corresponds to a user-provided version name
- **Persistent Index:** Index is built once and reused for all queries

### Query System
- **Word Count Query:** Returns the frequency of a given word in a specified version
- **Top-K Query:** Displays the top-K most frequent words sorted by frequency
- **Difference Query:** Computes the difference in word frequency between two versions

### Robustness
- **Exception Handling:** Descriptive error messages for all invalid inputs
- **Input Validation:** All command-line arguments are validated before processing
- **Case-Insensitive Matching:** All word matching is case-insensitive

## Implementation Details

### Class Structure
- **Buffer Class:** Manages fixed-size buffered reading of input files using RAII with `unique_ptr`
- **Tokenizer Class:** Extracts alphanumeric words from buffer chunks and handles split tokens across boundaries
- **VersionIndex Class:** Stores and manages word-frequency maps per version using a nested map structure
- **Query Class:** Abstract base class with pure virtual `compute()` function
- **WordCountQuery Class:** Derived from Query, implements word frequency lookup
- **TopKQuery Class:** Derived from Query, implements top-K frequent word retrieval
- **DifferenceQuery Class:** Derived from Query, implements cross-version frequency comparison

### C++ Features Demonstrated
- **Inheritance:** Abstract base class `Query` with three derived classes
- **Runtime Polymorphism:** `unique_ptr<Query>` with virtual dispatch via `compute()`
- **Function Overloading:** `getWordCounts()` overloaded with different parameter lists
- **Templates:** `getValue<T>()` generic map lookup function
- **Exception Handling:** `try/catch/throw` used throughout for robust error management
- **RAII:** `unique_ptr<char[]>` for automatic buffer memory management

## Installation and Setup

### Prerequisites
- C++ compiler with C++14 support (GCC, Clang, MSVC, etc.)
- Basic knowledge of command-line interface

### Dataset Setup
The program requires two standard text dataset files to run. Both are included in this repository:
- **test_logs.txt** — Dataset 1
- **verbose_logs.txt** — Dataset 2

Simply clone the repository and both files will be available in the project directory automatically.

> **Note:** The datasets must not be modified in any way.

### Installation Steps
1. Clone the repository or download the source code
2. Download both dataset files from the repository and place them in the project directory
3. Navigate to the project directory
4. Compile the source code:
```bash
g++ -std=c++14 -o analyzer yourfile.cpp
```
5. Run the compiled executable with appropriate arguments

## Usage Guide

### Word Count Query
Returns the frequency of a specific word in a given version:
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 512 --query word --word error
```

### Top-K Query
Returns the top K most frequent words in a given version:
```bash
./analyzer --file dataset_v1.txt --version v1 --buffer 512 --query top --top 10
```

### Difference Query
Returns the difference in word frequency between two versions:
```bash
./analyzer --file1 dataset_v1.txt --version1 v1 --file2 dataset_v2.txt --version2 v2 --buffer 512 --query diff --word error
```

## Command-Line Arguments

| Argument | Description |
|----------|-------------|
| `--file <path>` | Path to input file (single-version queries) |
| `--file1 <path>` | First input file (diff query) |
| `--file2 <path>` | Second input file (diff query) |
| `--version <n>` | Version identifier (single-version queries) |
| `--version1 <n>` | First version identifier (diff query) |
| `--version2 <n>` | Second version identifier (diff query) |
| `--buffer <kb>` | Buffer size in kilobytes (must be between 256 and 1024) |
| `--query <type>` | Query type: `word`, `top`, or `diff` |
| `--word <token>` | Word to search (word/diff queries) |
| `--top <k>` | Number of top results (top query) |

## Sample Output

```
Version name: v1
The count of the word 'error' in version v1 is: 605079
Buffer size: 512 KB

Execution time: 7.710 s
```

## Rules and Constraints

### Buffer Rules
- Buffer size must be between 256KB and 1024KB
- Buffer size remains constant throughout execution
- Words split across buffer boundaries are correctly reconstructed

### Word Rules
- Words are defined as contiguous sequences of alphanumeric characters
- All word matching is case-insensitive
- Non-alphanumeric characters act as word delimiters

### Memory Rules
- The entire file is never loaded into memory
- Memory usage grows only with the number of unique words
- Memory usage is independent of file size

## Troubleshooting

### Common Issues and Solutions
- **Missing File Path:** Ensure `--file` or `--file1/--file2` arguments point to valid files
- **Buffer Out of Range:** Buffer size must be strictly between 256 and 1024 KB
- **Missing Arguments:** All arguments are required — the program will report exactly which one is missing
- **Invalid Query Type:** Only `word`, `top`, and `diff` are supported query types
- **Word Not Found:** Returns a count of 0 — this is expected behavior, not an error

## Conclusion
This Memory-Efficient Versioned File Indexer provides a robust solution for processing and querying large text files without excessive memory usage. With features like incremental buffer-based reading, versioned indexing, and multiple query types, it addresses the core requirements of scalable file processing while maintaining clean object-oriented design.

The modular design with distinct classes for different responsibilities allows for easy maintenance and extension of the system as requirements evolve.

Thank you for using the Memory-Efficient Versioned File Indexer! Should you have any questions or encounter any issues, please don't hesitate to reach out.

This program was developed by Joy Vardhan Yalla as part of CS253 Course Programming Assignment offered in Semester 2025-26/II at IIT Kanpur.
