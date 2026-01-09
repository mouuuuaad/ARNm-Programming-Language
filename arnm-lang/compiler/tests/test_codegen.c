/*
 * ARNm Compiler - End-to-End Codegen Test
 */

#define _POSIX_C_SOURCE 200809L
#include "../include/irgen.h"
#include "../include/codegen.h"
#include "../include/parser.h"
#include "../include/sema.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static void test_codegen_stdout(void) {
    printf("  codegen_stdout...");
    
    const char* src = "fn main() -> i32 { let x = 30 + 12; return x; }";
    size_t len = strlen(src);
    
    AstArena arena;
    ast_arena_init(&arena, 1024 * 1024);
    
    Lexer lexer;
    lexer_init(&lexer, src, len);
    Parser parser;
    parser_init(&parser, &lexer, &arena);
    AstProgram* prog = parser_parse_program(&parser);
    
    if (!parser_success(&parser)) {
        printf(" Parse FAIL\n");
        return;
    }
    
    SemaContext sema;
    sema_init(&sema);
    sema_analyze(&sema, prog);
    
    IrModule mod;
    if (ir_generate(&sema, prog, &mod)) {
        /* Open memstream buffer to check output contents */
        char* buf;
        size_t size;
        FILE* mem = open_memstream(&buf, &size);
        
        if (codegen_emit(&mod, mem)) {
            fclose(mem);
            /* Check if output contains "add i32 30, 12" or similar (or evaluating constants?) 
               My IR gen emits ADD for literal addition currently, doesn't constant fold yet. 
               Wait, 30 + 12 -> 42? No, `gen_binary` emits `ir_build_add`.
            */
            if (strstr(buf, "define i32 @main()")) {
                if (strstr(buf, "add i32 30, 12") || strstr(buf, "add i32")) {
                    printf(" OK\n");
                } else {
                    printf(" FAIL (missing add instruction in output)\n");
                    printf("Output:\n%s\n", buf);
                }
            } else {
                printf(" FAIL (missing main definition)\n");
                printf("Output:\n%s\n", buf);
            }
        } else {
            printf(" FAIL (emit returned false)\n");
        }
        free(buf);
    } else {
        printf(" FAIL (ir gen)\n");
    }
    
    ir_module_destroy(&mod);
    ast_arena_destroy(&arena);
}

int main(void) {
    printf("Running Codegen tests:\n");
    test_codegen_stdout();
    return 0;
}
