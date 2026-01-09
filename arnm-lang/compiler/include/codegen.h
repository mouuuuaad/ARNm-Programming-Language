/*
 * ARNm Compiler - LLVM Code Generation Interface
 */

#ifndef ARNM_CODEGEN_H
#define ARNM_CODEGEN_H

#include "ir.h"
#include <stdbool.h>
#include <stdio.h>

/*
 * configuration for code generator.
 */
typedef struct {
    const char* filename; /* Output filename (.ll) */
} CodegenConfig;

/*
 * Emit LLVM IR for the module to the specified file.
 */
bool codegen_emit(IrModule* mod, FILE* out);

#endif /* ARNM_CODEGEN_H */
