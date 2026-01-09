---
description: How to build the ARNm compiler and runtime
---

This workflow details the steps to build the entire ARNm project from source.

1.  **Clean previous builds** (optional but recommended for fresh builds):
    ```bash
    make clean && make -C runtime clean
    ```

2.  **Build the Compiler (`arnmc`)**:
    // turbo
    ```bash
    make
    ```
    This produces the `build/arnmc` binary.

3.  **Build the Runtime Library**:
    // turbo
    ```bash
    make -C runtime
    ```
    This produces `runtime/build/libarnm.a` and `runtime/build/crt0.o`.

4.  **Verify Build**:
    Check if the compiler binary exists:
    ```bash
    ls -l build/arnmc
    ```
