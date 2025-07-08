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
*   } CatchCustom(...) {
*       ...
*   } CatchAll {
*       ...
*   } Finally {
*       ...
*   } End;
*
* NOTES:
*   - Chaining multiple 'Catch' and 'CatchCustom' blocks is supported.
*   - 'CatchAll', 'Finally', and 'CatchCustom' are optional.
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
typedef struct __exp_frame_t{
    short flag;                  // Flag to ensure 'Finally' block executes only once.
    int error_code;              // Stores the exception code if one is thrown.
    struct __exp_frame_t* prev;  // Pointer to the previous (outer) exception frame.
    jmp_buf buf;                 // The buffer to store the execution context for longjmp.
} __exp_frame;

// A thread-local pointer to the top of the exception frame stack.
// This is the key to making the library thread-safe.
thread_local static __exp_frame* __exp_stack_top = NULL;

// A thread-local struct to store details (file, function, line) for uncaught exceptions.
thread_local static struct{
    const char* file;
    const char* func;
    int line;
} __exception_detail_s = {0,0,0};

// A thread-local function pointer for a custom terminate handler.
// If set, it will be called for uncaught exceptions instead of the default behavior.
thread_local static const void (*__terminate_handle)(int) = NULL;

/**
* @brief Sets a custom handler function for uncaught exceptions.
* @param terminate_handle A function pointer that takes an integer (the error code) and returns void.
*                         The handler should not return. Pass NULL to reset to default.
*/
void set_exception_terminate_handle(void (*terminate_handle)(int)){
    __terminate_handle = terminate_handle;
}

/**
* @brief Internal function to handle the actual throwing logic.
*        It's not meant to be called directly by the user.
* @param code The integer exception code to be thrown.
*/
void __exp_throw_internal(int code){
    if (__exp_stack_top){
        // If we are inside a Try block, store the error code and jump.
        __exp_stack_top->error_code = code;
        longjmp(__exp_stack_top->buf,1);
    } else{
        // If a custom terminate handler is set, call it.
        if (__terminate_handle) __terminate_handle(code);
        // If no Try block is active and no custom handler is set (or it returns),
        // this is an uncaught exception. Print details and abort the program.
        printf("\n--- UNCAUGHT EXCEPTION ---\n"
            "Error Code: %d\n"
            "At -> %s\n"
            "Func -> %s\n"
            "Line -> %d\n"
            "--- PROGRAM WILL ABORT ---\n",
            code,__exception_detail_s.file,__exception_detail_s.func,__exception_detail_s.line);
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

// A convenience macro to access the current exception code within a CatchCustom block.
#define ErrorCode __e_frame.error_code

// Catches an exception based on a custom user-defined condition.
// This provides advanced, flexible exception matching beyond simple equality.
// The condition can be any valid C expression that evaluates to true or false.
// It's recommended to use the 'ErrorCode' macro to access the thrown error code.
// Example: CatchCustom(IS_FILE_ERROR(ErrorCode))
#define CatchCustom(condition) \
        } else if (((__e_frame.flag & 3) < 2) && (condition)) { \
            __e_frame.error_code = 0; /* Mark as handled */

// Catches a specific exception by its error code.
#define Catch(e) \
        } else if (__e_frame.error_code == (e) && ((__e_frame.flag & 3) < 2)) { \
            __e_frame.error_code = 0; /* Mark as handled */

// Catches any remaining unhandled exceptions.
#define CatchAll \
        } else if((__e_frame.flag & 3) < 2){ \
            __e_frame.error_code = 0; /* Mark as handled */

// Defines a block of code that will always execute, regardless of whether an exception was thrown.
#define Finally \
        } \
        if (!(__e_frame.flag & 4)) { \
            __e_frame.flag |= 4;

// Ends the exception block. Pops the frame and re-throws if an error was not handled.
#define End \
        } \
        __exp_stack_top = __e_frame.prev; \
        if (__e_frame.error_code != 0) { \
           if (__exp_stack_top) ++__exp_stack_top->flag;\
            __exp_throw_internal(__e_frame.error_code); \
        } \
    } while(0)

// Throws an exception with a given error code.
// It captures the file, function, and line number where the exception is thrown.
#define Throw(e) \
    do { \
        __exception_detail_s.line = __LINE__; \
        __exception_detail_s.file = __FILE__; \
        __exception_detail_s.func = __FUNCTION__; \
        if (__exp_stack_top) ++__exp_stack_top->flag;\
        __exp_throw_internal(e); \
    } while(0)

// Special macros to exit from a Try block, bypassing Finally.
// WARNING: These are for performance-critical paths. Manual resource cleanup is required.
#define Return   { __exp_stack_top = __e_frame.prev; return; }
#define Break    { __exp_stack_top = __e_frame.prev; break; }
#define Continue { __exp_stack_top = __e_frame.prev; continue; }

#endif // !__TINY_C_EXCEPTION_H