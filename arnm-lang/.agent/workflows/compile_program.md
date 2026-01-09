---
description: How to compile and link an ARNm program
---

This workflow explains the manual linking process required to create an executable from an `.arnm` source file.

**Prerequisites**: Ensure the project is built (see `build_project.md`).

1.  **Compile to Assembly**:
    Use `arnmc` to generate x86_64 assembly.
    ```bash
    ./build/arnmc --emit-asm path/to/your/file.arnm > build/output.s
    ```

2.  **Assemble**:
    Use `gcc` (or `as`) to assemble the output into an object file.
    ```bash
    gcc -c build/output.s -o build/output.o
    ```

3.  **Link**:
    Link the object file with the ARNm runtime (`libarnm.a`) and entry point (`crt0.o`).
    ```bash
    gcc -o my_program runtime/build/crt0.o build/output.o -Lruntime/build -larnm -lpthread
    ```

4.  **Run**:
    ```bash
    ./my_program
    ```
