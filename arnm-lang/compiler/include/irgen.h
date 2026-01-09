/*
 * ARNm Compiler - IR Generation Interface
 * 
 * Transforms the typed AST into SSA IR.
 */

#ifndef ARNM_IRGEN_H
#define ARNM_IRGEN_H

#include "ir.h"
#include "ast.h"
#include "sema.h"

/*
 * Generates IR for the entire program.
 * 
 * @param ctx       Semantic analysis context (for types/symbols)
 * @param program   Root AST node (AstProgram)
 * @param out_mod   Output IR module
 * @return          true on success
 */
bool ir_generate(SemaContext* ctx, AstProgram* program, IrModule* out_mod);

#endif /* ARNM_IRGEN_H */
