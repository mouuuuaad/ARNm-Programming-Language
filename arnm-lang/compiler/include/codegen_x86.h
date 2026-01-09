#ifndef ARNM_CODEGEN_X86_H
#define ARNM_CODEGEN_X86_H

#include "ir.h"
#include <stdio.h>

/* Emit x86_64 assembly for the given IR module */
void x86_emit(IrModule* mod, FILE* out);

#endif /* ARNM_CODEGEN_X86_H */
