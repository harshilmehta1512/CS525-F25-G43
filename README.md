📦 Storage Manager – Assignment 1  
Course: CS525 – Advanced Database Organization  
Semester: Fall 2025  
Authors: Harshil Mehta and Radhika Sharma  
Institute: Illinois Institute of Technology  

🧭 Project Overview  

This project is an implementation of a page-based storage manager in C. The manager operates on files as collections of fixed-sized pages (4096 bytes per page). It provides the core operations needed for file and block handling:  

- Creation, opening, closing, and deletion of page files  
- Block-level read and write operations  
- Appending empty pages at the end of a file  
- Expanding file size dynamically to ensure minimum capacity  
- Automated testing to confirm correctness and robustness  

The project consists of the core storage manager code, error handling utilities, and two test drivers that execute both the professor’s baseline tests and additional custom cases.  


📁 Directory Structure  

assign1_storage_manager/
│
├── storage_mgr.c          # Core storage manager implementation
├── storage_mgr.h          # Public interface for page file management
├── dberror.c              # Error handling functions
├── dberror.h              # Error codes and macros
├── test_helper.h          # Assertion and logging macros
├── test_assign1_1.c       # Baseline professor-provided tests
├── integrated_tester.c    # Unified test runner (baseline + custom validation)
├── Main_testing_file.c    # Alternate runner with a different extended test set
├── makefile               # Build automation for compiling & running tests
└── README.md              # Documentation


⚙️ Building and Running the Project  

The project is managed with a `makefile` . To compile the source code and run all test cases in one step, simply run the following command:

make run

This command will:  
- Compile the core modules (`dberror.c` and `storage_mgr.c`)  
- Build two executables:  
  - `integrated_tester`  
  - `Main_testing_file`  
- Apply the necessary execution permissions (`chmod +x`) automatically  
- Ensures both binaries 
- Save their results into: 
  - integrated_tester_output.txt
  - Main_testing_file_output.txt  

To check results:
cat integrated_tester_output.txt
cat main_testing_file_output.txt

To clean up compiled artifacts (executables and any generated output files) run the following command:

make clean

✅ Test Suite Coverage  

Baseline Tests (from `test_assign1_1.c`)  
- Create, open, close, and destroy a page file  
- Confirm that a newly created file contains exactly one zero-filled page  
- Write patterned data to a page, read it back, and verify integrity  

Extended Tests (`integrated_tester.c`)  
- Capacity expansion test: grow a file to a specified number of pages using `ensureCapacity`  
- Validation of last page read/write with predictable data patterns  

Alternate Extended Tests (`Main_testing_file.c`)  
- Stepwise block appending followed by writes to the last page  
- Mid-block writes with cursor (`curPagePos`) validation to ensure correct positioning  

📄 Example Output of integrated_tester file:

```
=== Running integrated_tests ===
[./test_assign1_1.c-test create open and close methods-L47-20:23:28] OK: expected true: filename correct
[./test_assign1_1.c-test create open and close methods-L48-20:23:28] OK: expected true: expect 1 page in new file
[./test_assign1_1.c-test create open and close methods-L49-20:23:28] OK: expected true: freshly opened file's page position should be 0
[./test_assign1_1.c-test create open and close methods-L55-20:23:28] OK: expected true: opening non-existing file should return an error.
[./test_assign1_1.c-test create open and close methods-L57-20:23:28] OK: finished test

created and opened file
[./test_assign1_1.c-test single page content-L81-20:23:28] OK: expected true: expected zero byte in first page of freshly initialized page


[integrated_tester.c-A: ensureCapacity + write/read last page-L56-20:23:28] OK: expected true: A: buffer alloc
[integrated_tester.c-A: ensureCapacity + write/read last page-L78-20:23:28] OK: expected true: A: capacity reached
[integrated_tester.c-A: ensureCapacity + write/read last page-L50-20:23:28] OK: expected true: A: last-page pattern ok
[integrated_tester.c-A: ensureCapacity + write/read last page-L93-20:23:28] OK: finished test

[integrated_tester.c-B: appendEmptyBlock growth + interior page verification-L56-20:23:28] OK: expected true: B: buffer alloc
[integrated_tester.c-B: appendEmptyBlock growth + interior page verification-L113-20:23:28] OK: expected true: B: total pages updated after appends
[integrated_tester.c-B: appendEmptyBlock growth + interior page verification-L50-20:23:28] OK: expected true: B: mid-page pattern ok
[integrated_tester.c-B: appendEmptyBlock growth + interior page verification-L128-20:23:28] OK: expected true: B: curPagePos points at last-read page
[integrated_tester.c-B: appendEmptyBlock growth + interior page verification-L134-20:23:28] OK: finished test

```

📄 Example Output of Main_testing_file:

```
=== Running main_testing_file ===
== Main_tester runner ==
-- Running professor baseline tests --
[./test_assign1_1.c-test create open and close methods-L47-20:23:28] OK: expected true: filename correct
[./test_assign1_1.c-test create open and close methods-L48-20:23:28] OK: expected true: expect 1 page in new file
[./test_assign1_1.c-test create open and close methods-L49-20:23:28] OK: expected true: freshly opened file's page position should be 0
[./test_assign1_1.c-test create open and close methods-L55-20:23:28] OK: expected true: opening non-existing file should return an error.
[./test_assign1_1.c-test create open and close methods-L57-20:23:28] OK: finished test

created and opened file
[./test_assign1_1.c-test single page content-L81-20:23:28] OK: expected true: expected zero byte in first page of freshly initialized page

-- Running custom extended tests --
[Main_testing_file.c-XT1: capacity growth and last-page roundtrip-L55-20:23:28] OK: expected true: XT1 alloc
[Main_testing_file.c-XT1: capacity growth and last-page roundtrip-L76-20:23:28] OK: expected true: XT1: capacity reached target
[Main_testing_file.c-XT1: capacity growth and last-page roundtrip-L49-20:23:28] OK: expected true: XT1: last-page pattern ok
[Main_testing_file.c-XT1: capacity growth and last-page roundtrip-L89-20:23:28] OK: finished test

[Main_testing_file.c-XT2: append sequence and mid-block check-L55-20:23:28] OK: expected true: XT2 alloc
[Main_testing_file.c-XT2: append sequence and mid-block check-L108-20:23:28] OK: expected true: XT2: page count after appends
[Main_testing_file.c-XT2: append sequence and mid-block check-L49-20:23:28] OK: expected true: XT2: mid-page pattern ok
[Main_testing_file.c-XT2: append sequence and mid-block check-L119-20:23:28] OK: expected true: XT2: curPagePos reflects mid page
[Main_testing_file.c-XT2: append sequence and mid-block check-L125-20:23:28] OK: finished test

== All tests finished (baseline rc=0) ==

```

---

Link for the explanation video:

https://iit0-my.sharepoint.com/:v:/g/personal/hmehta26_hawk_illinoistech_edu/EbEVAavdhodPhe9MRC4Y48oB_XrIFO-_3TVQg-8rjrea6Q?e=BvEDi2

📬 Contact  

Harshil Mehta  
📧 Email: hmehta26@hawk.illinoistech.edu  

**Radhika Sharma**  
📧 Email: rsharma20@hawk.illinoistech.edu
