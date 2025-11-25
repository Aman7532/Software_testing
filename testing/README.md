# Fuzz Testing Project - Configuration File Parser
CSE 731 - Software Testing

## Team Members
- Aman Agarwal (MT2024017)
- Satyam Gupta (MT2024134)

---

## AI Tool Usage Declaration

We used **Claude AI (Anthropic)** in this project. Being honest about this:

**What Claude helped us with:**
- Helping with the configuration parser source code (config_parser.c and config_parser.h)
- Explaining and implementing the parser logic (functions, data structures, memory management)
- Understanding how fuzz testing works and how AFL++ operates
- Creating the test input files (corpus)
- Setting up AFL++ on macOS and fixing compilation errors
- Writing the Makefile and run_fuzzer.sh script
- Understanding ASAN error messages and what the crashes mean
- Structuring the project and organizing files

**What we did ourselves:**
- Running the fuzzing campaigns
- Analyzing the results and understanding what crashes mean
- Taking screenshots and documenting the process
- Learning about fuzzing and explaining it to each other
- Making decisions about test strategy and corpus design
- Testing and verifying everything works

Basically, we used Claude as a teacher/tutor to learn fuzzing and implement this project. We couldn't have written 1000+ lines of C code in a few days without help. But we understand how everything works and can explain and demonstrate it.

---

## What This Project Does

This is a fuzz testing project. We test a configuration file parser (written in C) using AFL++ fuzzer to find bugs automatically.

The parser can read config files (like .ini files) with:
- Key-value pairs: `name = value`
- Different types: strings, numbers, booleans, arrays
- Sections: `[database]` or `[server]`
- Comments: `# this is a comment`
- Arrays: `ports = [8080, 8081, 8082]`

We test it using AFL++ which is a fuzzer - it automatically generates tons of random/mutated inputs and tries to crash the program. The idea is if it crashes, we found a bug!

**Tools we used:**
- AFL++ v4.35a (the fuzzer itself)
- AddressSanitizer (ASAN) - catches memory bugs like buffer overflows
- UndefinedBehaviorSanitizer (UBSAN) - catches other undefined behavior
- LLVM/Clang compiler on macOS

**Why fuzz testing?**
Because manually writing test cases is slow and boring. AFL++ can run thousands of tests per minute automatically and finds bugs we would never think to test for.

## Source Code

**Files:**
- `src/config_parser.c` - 956 lines (main implementation)
- `src/config_parser.h` - 118 lines (header/definitions)
- Total: 1,074 lines of code

**What the parser does:**
Reads config files and parses them into a data structure. It handles different data types (strings, ints, floats, booleans, arrays), supports sections like [database], and has error handling for malformed inputs.

**Main functions in the code:**
- `parser_init()` - starts up the parser
- `parse_file()` - reads the config file
- `parse_line()` - processes each line
- `parse_value()` - figures out what type the value is
- `parse_array()` - handles arrays like [1,2,3]
- `trim_whitespace()` - cleans up strings
- `get_value()` - retrieves values from the parsed config

---

## How We Tested

### Test Inputs (Corpus)
We made 18 test files to feed to AFL++:

**Valid config files (11 files)** - These are properly formatted:
- `valid_simple.txt` - basic stuff like `key = value`
- `valid_arrays.txt` - testing arrays
- `valid_sections.txt` - testing [section] headers
- `valid_database_config.txt`, `valid_server_config.txt`, `valid_application_settings.txt`, etc. - different realistic config files

**Edge cases (4 files)** - weird but possibly valid:
- `edge_empty.txt` - empty file
- `edge_long_values.txt` - really long strings
- `edge_nested_sections.txt` - deeply nested sections
- `edge_special_chars.txt` - special characters

**Boundary tests (3 files)** - stress testing:
- `boundary_numbers.txt` - huge numbers
- `stress_many_entries.txt` - lots of key-value pairs
- `valid_types.txt` - mix of everything

**Why mostly valid inputs?**
We learned that AFL++ works better if you give it mostly VALID inputs. It can successfully parse them and explore deep into the code, then it mutates them to find bugs. If you start with all invalid/broken inputs, the parser just rejects them immediately and you don't find deep bugs.

### What AFL++ Does
AFL++ compiles our code with special instrumentation that tracks:
- Which parts of code get executed
- Which branches (if/else) are taken
- Which paths through the program are explored

Then it mutates the inputs intelligently - if a mutation reaches new code, it keeps it and mutates it more. If it doesn't reach new code, it discards it. This is called "coverage-guided" fuzzing.

### How We Catch Bugs
We compiled with:
- **ASAN** (AddressSanitizer) - catches memory bugs like buffer overflows, null pointer crashes, use-after-free
- **UBSAN** (UndefinedBehaviorSanitizer) - catches integer overflows and other undefined behavior

When the program crashes, ASAN tells us exactly what went wrong and where.

---

## What's in the Submission

```
README.md                       - this file
Makefile                        - builds everything
run_fuzzer.sh                   - runs AFL++
src/
  ├── config_parser.c           - main code (956 lines)
  └── config_parser.h           - header (118 lines)
tests/
  ├── corpus/                   - 18 test input files
  └── output/default/
      ├── crashes/              - 35 crash files!
      ├── queue/                - 278 test cases AFL++ made
      └── fuzzer_stats          - statistics
build/
  ├── config_parser_fuzz        - instrumented binary
  └── config_parser_asan        - ASAN binary
screenshots/                    - screenshots of AFL++
fuzzing_1hour_log.txt          - full log of fuzzing run
```

---

## How to Run This

### Setup (one time)

Install LLVM:
```bash
brew install llvm
```

Get AFL++:
```bash
git clone https://github.com/AFLplusplus/AFLplusplus
cd AFLplusplus
make
cd ..
```

### Build

```bash
make fuzz          # builds the instrumented binary
make asan          # builds ASAN-only version for reproducing crashes
make clean         # cleans up
```

### Run Fuzzing

```bash
./run_fuzzer.sh 3600     # runs for 1 hour
./run_fuzzer.sh 600      # or 10 minutes for demo
```

### Check Results

```bash
# see stats
cat tests/output/default/fuzzer_stats

# count crashes
ls tests/output/default/crashes/id:* | wc -l

# reproduce a crash
make reproduce CRASH=tests/output/default/crashes/id:000000
```

---

## Results

We ran AFL++ for 1 hour. Here's what happened:

- Tests run: 84,153 
- Speed: about 23 tests/second
- New test cases AFL++ created: 260 (we started with 18, ended with 278)
- Code coverage: 31.49%
- **Crashes: 35!**
- Hangs: 0

### Bugs Found

AFL++ found 35 crashes, all memory bugs:

**Bug #1: Buffer Overflow**
- Function: `trim_whitespace()` (line 234)
- Cause: really long strings (1000+ bytes) overflow the buffer
- ASAN error: `heap-buffer-overflow`

**Bug #2: Null Pointer Crash**  
- Somewhere in key handling
- Cause: certain key patterns trigger null pointer dereference
- ASAN error: `SEGV on unknown address`

**Bug #3: Array Overflow**
- Function: `parse_array()`
- Cause: arrays with 100+ elements overflow the array buffer
- ASAN error: `heap-buffer-overflow`

All these are critical security bugs.

### Example Crash Output

One crash file (`id:000000`) has this ASAN output:

```
==46233==ERROR: AddressSanitizer: heap-buffer-overflow
    #0 trim_whitespace src/config_parser.c:234
    #1 parse_line src/config_parser.c:456
    #2 main src/config_parser.c:890
```

Screenshots are in the `screenshots/` folder.

---

## Tools Used

**AFL++** (v4.35a) - https://github.com/AFLplusplus/AFLplusplus  
This is the fuzzer. It's open source (Apache 2.0 license). Used by Google, Microsoft, etc. for finding bugs.

**ASAN** (AddressSanitizer)  
Comes with LLVM/Clang. Catches memory bugs at runtime.

**UBSAN** (UndefinedBehaviorSanitizer)  
Also from LLVM/Clang. Catches integer overflows and other undefined behavior.

**Make + Clang**  
For building the code.

---

## Who Did What

**Aman Agarwal (MT2024017):**

Worked on these code sections (with Claude's help):
- `trim_whitespace()` function - removes spaces from strings
- `is_valid_key()` function - checks if a key name is valid
- `print_config()` and `print_value()` functions - prints parsed config
- `get_string()`, `get_int()`, `get_bool()` functions - retrieve values from config
- Created test input files: `valid_simple.txt`, `valid_arrays.txt`, `edge_empty.txt`, etc.

Other work:
- Ran the 1-hour fuzzing campaign
- Wrote `run_fuzzer.sh` script with Claude
- Analyzed crash outputs to understand the bugs
- Helped write the Makefile with Claude
- Tested the build process

**Satyam Gupta (MT2024134):**

Worked on these code sections (with Claude's help):
- `parser_init()` and `parser_free()` functions - memory management
- `parse_file()` and `parse_line()` functions - main parsing logic
- `parse_value()` function - detects data types (string/int/float/bool/array)
- `parse_array()` function - parses arrays like [1,2,3]
- `create_entry()`, `add_entry()`, `free_entry()` functions - manage config entries
- Created test files: `valid_sections.txt`, `boundary_numbers.txt`, `stress_many_entries.txt`, etc.

Other work:
- Set up AFL++ on macOS and fixed compilation errors
- Reproduced crashes using `make reproduce`
- Analyzed ASAN output messages
- Helped document everything
- Took screenshots of AFL++ running

---

## What We Achieved

- 1,074 lines of C code (requirement was 1000+)
- Used AFL++, an industry-standard tool
- Found 35 real crashes/bugs
- Ran 84,153 automated tests
- Got 31.49% code coverage
- Everything is reproducible

## How to Demo

Show the code:
```bash
wc -l src/config_parser.c src/config_parser.h
```

Show test inputs:
```bash
ls tests/corpus/
cat tests/corpus/valid_simple.txt
```

Show results:
```bash
cat tests/output/default/fuzzer_stats | head -20
ls tests/output/default/crashes/
```

Reproduce a crash:
```bash
make reproduce CRASH=tests/output/default/crashes/id:000000
```

## Summary

We used AFL++ to fuzz test a configuration parser. In 1 hour it ran 84,000+ tests and found 35 crashes. All the crashes are (buffer overflows, null pointers, etc.) detected by ASAN.

This was a good learning experience about automated testing and how fuzzers work. AFL++ found bugs we would never have thought to test for manually.

---

**Team:** Aman Agarwal (MT2024017), Satyam Gupta (MT2024134)  
**Course:** CSE 731 - Software Testing  

