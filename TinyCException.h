#ifndef __TINY_C_EXCEPTION_H
#define __TINY_C_EXCEPTION_H

#include <setjmp.h>
#include <stdio.h>
#include <threads.h>
#include <stdlib.h>

/*
* TinyCException - A modern, header-only, thread-safe exception handling library for C11.
*
* COMPILER:
*   Requires a C11 compliant compiler or newer.
*
* SYNTAX:
*   Try {
*       ...
*       Throw(e);
*       ...
*   } Catch(e1) {
*       ...
*   } Catch(e2) {
*       ...
*   } CatchAll {
*       ...
*   } Finally {
*       ...
*   } End;
*
* NOTES:
*   - Chaining multiple 'Catch' blocks is supported.
*   - 'CatchAll' and 'Finally' are optional.
*   - The exception code 'e' must be a non-zero integer.
*   - Do not use 'goto' to jump across scopes within an exception block.
*   - Use 'volatile' for local variables modified in 'Try' if they are accessed in 'Catch'.
*   - The 'Return', 'Break', and 'Continue' macros bypass the 'Finally' block for performance.
*     Manual resource cleanup is required before using them.
*   - The error_code is stored on the stack frame rather than a global thread_local variable
*     to improve performance, safety, and readability.
*/

// The exception frame structure.
// It's a linked list node, forming a stack of exception contexts for each thread.
typedef struct __exp_frame_t {
    short flag;                  // Flag to ensure 'Finally' block executes only once.
    int error_code;              // Stores the exception code if one is thrown.
    struct __exp_frame_t* prev;  // Pointer to the previous (outer) exception frame.
    jmp_buf buf;                 // The buffer to store the execution context for longjmp.
} __exp_frame;

// A thread-local pointer to the top of the exception frame stack.
// This is the key to making the library thread-safe.
thread_local static __exp_frame* __exp_stack_top = NULL;

// A thread-local array to store details (file, function, line) for uncaught exceptions.
thread_local static const char* __exception_detail[3] = {0, 0, 0};

// Internal function to handle the actual throwing logic.
void __exp_throw_internal(int code) {
    if (__exp_stack_top) {
        // If we are inside a Try block, store the error code and jump.
        __exp_stack_top->error_code = code;
        longjmp(__exp_stack_top->buf, 1);
    } else {
        // If no Try block is active, this is an uncaught exception.
        // Print details and abort the program.
        printf("\n--- UNCAUGHT EXCEPTION ---\n"
            "Error Code: %d\n"
            "At -> %s\n"
            "Func -> %s\n"
            "Line -> %d\n"
            "--- PROGRAM WILL ABORT ---\n",
            code, __exception_detail[0], __exception_detail[1], (int)(__exception_detail[2]));
        fflush(stdout);
        abort();
    }
}

// Begins a protected block. Pushes a new exception frame onto the stack.
#define Try \
    do { \
        __exp_frame __e_frame; \
        __e_frame.prev = __exp_stack_top; \
        __exp_stack_top = &__e_frame; \
        __e_frame.error_code = 0; \
        __e_frame.flag = 0; \
        if (setjmp(__e_frame.buf) == 0) {

// Catches a specific exception by its error code.
#define Catch(e) \
        } else if (__e_frame.error_code == (e)) { \
            __e_frame.error_code = 0; // Mark as handled

// Catches any remaining unhandled exceptions.
#define CatchAll \
        } else { \
            __e_frame.error_code = 0; // Mark as handled

// Defines a block of code that will always execute.
#define Finally \
        } \
        if (!__e_frame.flag) { \
            __e_frame.flag = 1;

// Ends the exception block. Pops the frame and re-throws if an error was not handled.
#define End \
        } \
        __exp_stack_top = __e_frame.prev; \
        if (__e_frame.error_code != 0) { \
            __exp_throw_internal(__e_frame.error_code); \
        } \
    } while(0)

// Throws an exception with a given error code.
#define Throw(e) \
    do { \
        __exception_detail[0] = __FILE__; \
        __exception_detail[1] = __FUNCTION__; \
        __exception_detail[2] = (char*)__LINE__; \
        __exp_throw_internal(e); \
    } while(0)

// Special macros to exit from a Try block, bypassing Finally.
#define Return   { __exp_stack_top = __e_frame.prev; return; }
#define Break    { __exp_stack_top = __e_frame.prev; break; }
#define Continue { __exp_stack_top = __e_frame.prev; continue; }

#endif // !__TINY_C_EXCEPTION_H