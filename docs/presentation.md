---
marp: true
---

# Crafting Interpreters
## Michael Lisitsa - 16 March 2026

--- 
# Outline

- Demo showing it working.
    - Cool example programs that work
- Virtual machine bytecode
- Global hash table
    - Implemented from scratch. Each interned string has its own hash, max load, tombstone. Good fodder for next AOC.
    - Top-level functions
    - Top-level variables
- Value Stack
    - Local variables
    - Scopes. Flat array of constants, with a scope depth to track how many curlies you are in.
    - Nested functions
- Conditionals -> jump_if_false
- What is a value?
    - Constants: Known at compile time.
    - Fixed size, Numbers, booleans
    - Dynamically allocated, e.g. strings.
- Dynamic typing
    - Lexer will scan source code at runtime to determine type of string.
    - Type info in ObjType struct. `add` function will always check the type before dispatching to function.
        - Is it an object, get its type, Does it match `string`, if not is it a number? Issue with cache misses as following pointers.
- Garbage collector
   https://omrelli.ug/nearley-playground/ - reference counting (not implemented)
    - mark and sweep. Used in python just for circular dependencies.
        - Linked List to all allocated objects.
- Compiling vs interpreting
    - Code is called by `run` call in top-level interpret function, which may trigger OP_CALL opcodes. So all bytecode is compiled before any invocation.
- Interesting projects
    - Maths evaluation -> Math.JS. Uses Javascript call stack and closures to compile an AST into a Javascript function.
    - [Nearley](https://omrelli.ug/nearley-playground/) - Generic lexer written in JS.
