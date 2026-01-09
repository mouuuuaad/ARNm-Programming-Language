/*
 * ARNm Compiler - IR Gen Tests
 */

#include "../include/irgen.h"
#include "../include/parser.h"
#include "../include/sema.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void test_irgen_basic(void) {
    printf("  irgen_basic...");
    
    const char* src = "fn main() -> i32 { let x = 40 + 2; return x; }";
    size_t len = strlen(src);
    
    /* Arena */
    AstArena arena;
    ast_arena_init(&arena, 1024 * 1024);
    
    /* Parse */
    Lexer lexer;
    lexer_init(&lexer, src, len);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    AstProgram* prog = parser_parse_program(&parser);
    
    if (!parser_success(&parser)) {
        printf(" Parse Error!\n");
        return;
    }
    printf(" Decl count: %zu\n", prog->decl_count);
    
    /* Sema */
    SemaContext sema;
    sema_init(&sema);
    sema_analyze(&sema, prog);
    
    /* IR Gen */
    IrModule mod;
    bool ok = ir_generate(&sema, prog, &mod);
    
    if (ok) {
        if (mod.funcs) {
             printf(" Found func: %s\n", mod.funcs->name);
        } else {
             printf(" No funcs in module.\n");
        }
        /* Verify we have main function */
        if (mod.funcs && strcmp(mod.funcs->name, "main") == 0) {
            printf(" OK\n");
        } else {
            printf(" FAIL (missing main)\n");
        }
        // ir_dump_module(&mod);
    } else {
        printf(" FAIL (gen error)\n");
    }
    
    ir_module_destroy(&mod);
    ast_arena_destroy(&arena);
}

int main(void) {
    printf("Running IR Gen tests:\n");
    test_irgen_basic();
    return 0;
}
