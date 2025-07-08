
# TinyCException

A modern, header-only, thread-safe exception handling library for C11.

It provides an elegant `Try-Catch-Finally` syntax, allowing for more robust and readable error-handling code without the complexity of larger frameworks.

## Why TinyCException?

Because C deserves better error handling. Traditional error code propagation is tedious and clutters your logic. `TinyCException` offers a clean, centralized way to manage errors, inspired by modern programming languages.

## ✨ Features

-   **Intuitive Syntax**: `Try { ... } Catch(e) { ... } Finally { ... } End;`
-   **Flexible `Throw`**: Can be used anywhere for unified error handling.
-   **Header-Only**: Just `#include "TinyCException.h"`.
-   **Thread-Safe**: Designed for modern, concurrent applications.
-   **Lightweight & Fast**: Minimal overhead, built on C's `setjmp`/`longjmp`.
-   **Robust**: Supports nested `Try` blocks and provides detailed uncaught exception reports.

## 🚀 Quick Start

It's highly recommended to use an `enum` to manage your exception codes for better readability and maintainability.

```c
#include "TinyCException.h"
#include <stdio.h>

// Define your exception codes in an enum
typedef enum {
    None = 0, // Conventionally, 0 is not used for exceptions
    InvalidInput,
    FileError
} ExceptionCode;

void process_data(int input) {
    if (input < 0) {
        Throw(InvalidInput);
    }
    printf("Data processed successfully.\n");
}

int main() {
    Try {
        printf("Processing data...\n");
        process_data(-1); // This will throw
    } Catch(InvalidInput) {
        printf("Error: Invalid input detected.\n");
    } Catch(FileError) {
        printf("Error: A file error occurred.\n");
    } Finally {
        printf("Operation finished.\n");
    } End;

    return 0;
}
```

## 📖 API in Detail

#### `Try { ... }` & `Catch(e) { ... }`
The fundamental block. `Try` protects a code segment, and `Catch` handles a specific error.

```c
Try {
    Throw(42);
} Catch(42) {
    printf("Caught exception 42!\n");
} End;
```

#### `CatchCustom(condition)` ✨
For advanced users, `CatchCustom` allows you to catch exceptions based on a custom boolean condition. This enables powerful matching logic, such as catching a whole category of errors.

Inside the `condition`, you can use the `ErrorCode` macro to access the currently thrown exception code.

```c
// Let's define a category for file-related errors
#define IS_FILE_ERROR(code) ((code) >= 200 && (code) < 300)

// In your enum:
// FileError_NotFound = 201,
// FileError_PermissionDenied = 202,

Try {
    Throw(FileError_NotFound);
} Catch(SomeOtherError) {
    /* Skipped */
} CatchCustom(IS_FILE_ERROR(ErrorCode)) {
    // This block will catch any error code between 200 and 299
    printf("Caught a generic file error with code: %d\n", ErrorCode);
} End;
```

#### `CatchAll { ... }`
A fallback to catch any exception not handled by a specific `Catch`.

```c
Try {
    Throw(99);
} Catch(1) {
/* Skipped */ 
} CatchAll {
    printf("Caught an unexpected exception!\n");
} End;
```

#### `Finally { ... }`
Guarantees execution of cleanup code, whether an exception was thrown or not.

```c
Try {
    printf("Executing...\n");
} Finally {
    printf("Cleanup always runs.\n");
} End;
```

#### `Throw(e)`
Throws an exception. Can be used anywhere. If outside a `Try` block, it will terminate the program with a detailed report.

#### `Return`, `Break`, `Continue`
Special macros to exit a scope from within a `Try` block. **Warning: They bypass `Finally`!** Manual cleanup is required before use.

```c
for (int i = 0; i < 2; ++i) {
    Try {
        if (i == 0) Continue; // Skips Finally for this iteration
        printf("i = %d\n", i);
    } Finally {
        printf("Finally for i = %d\n", i);
    } End;
}
```

## ⚠️ Important Notes & Best Practices

This library grants you power and flexibility, but with great power comes great responsibility. Please be aware of the following:

1.  **Manual Resource Management**: `longjmp` does not unwind the stack. Memory allocated with `malloc` or other resources (like file handles) acquired in a `Try` block are **not** automatically released. **Always use the `Finally` block for all cleanup logic.** This is the most critical rule.

2.  **`volatile` Keyword**: If you modify a local variable within a `Try` block and need its updated value after a `Throw`, declare it as `volatile`. This prevents the compiler from over-optimizing and ensures the value in memory is correct.

3.  **`Throw` in `Catch` and `Finally`**:
    *   **In `Catch`**: You can `Throw` from a `Catch` block to re-throw an exception to an outer `Try-Catch` scope. This is useful for creating exception chains.
    *   **In `Finally` (Highly Discouraged)**: While technically possible, throwing from a `Finally` block is a bad practice. It will overwrite any pre-existing exception, making the original error impossible to trace. Avoid this unless you have a very specific reason.

4.  **Forbidden `goto`**: Do not use `goto` to jump into or out of a `Try-Catch` block. This will corrupt the exception stack and lead to undefined behavior.

5.  **Non-Zero Exception Codes**: The exception code `0` is reserved for "no exception". Do not `Throw(0)`.

## License

This project is licensed under the MIT License.
