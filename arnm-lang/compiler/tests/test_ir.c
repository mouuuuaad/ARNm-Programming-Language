/*
 * ARNm Compiler - IR Tests
 */

#include "../include/ir.h"
#include <stdio.h>
#include <assert.h>

static void test_simple_function(void) {
    printf("  simple_function...");
    
    IrModule mod;
    ir_module_init(&mod);
    
    /* fn main() -> i32 */
    IrFunction* fn = ir_function_create(&mod, "main", ir_type_i32(), NULL, 0);
    IrBlock* entry = ir_block_create(fn, "entry");
    
    /* %1 = 42 */
    /* ret %1 */
    IrValue val = ir_val_const_i32(42);
    /* For now, our const is an instruction less value, typically loaded.
       But let's simulate: %1 = add 40, 2 */
    
    IrValue c40 = ir_val_const_i32(40);
    IrValue c2 = ir_val_const_i32(2);
    
    IrInstr* add = ir_build_add(fn, entry, c40, c2);
    ir_build_ret(entry, add->result);
    
    /* Just verifying it doesn't crash and structure exists */
    assert(fn->entry == entry);
    assert(entry->head->op == IR_ADD);
    assert(entry->tail->op == IR_RET);
    
    /* Dump to stdout for manual check */
    // ir_dump_module(&mod);
    
    ir_module_destroy(&mod);
    printf(" OK\n");
}

int main(void) {
    printf("Running IR tests:\n");
    test_simple_function();
    return 0;
}
