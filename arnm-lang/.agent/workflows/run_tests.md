---
description: How to run the compiler test suite
---

This workflow describes how to run the automated tests for separate compiler phases.

1.  **Run All Tests**:
    The main Makefile has a `test` target that runs all registered test suites.
    // turbo
    ```bash
    make test
    ```

2.  **Run Specific Test Suites**:
    You can run tests for individual components:
    
    *   **Lexer**: `make test_lexer`
    *   **Parser**: `make test_parser`
    *   **Semantic Analysis**: `make test_sema`
    *   **IR Generation**: `make test_irgen`
    *   **Code Generation**: `make test_codegen`

3.  **Run Integration Verification**:
    To verify full end-to-end functionality (compiling and running a program), use the specific examples:
    
    ```bash
    ./build/arnmc --emit-asm examples/test_loops.arnm > build/test.s
    gcc -c build/test.s -o build/test.o
    gcc -o build/test runtime/build/crt0.o build/test.o -Lruntime/build -larnm -lpthread
    ./build/test
    ```
