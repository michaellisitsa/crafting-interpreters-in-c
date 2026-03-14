---
marp: true
style: |
  .columns {
    display: flex;
    align-items: center;
    gap: 1em;
  }
  .columns > div {
    flex: 1;
  }
  .reveal { opacity: 0; transition: opacity 0.3s; }
  .reveal:hover { opacity: 1; }
  .green {color: green;}
  .red {color: red;}
---

# Crafting Interpreters
## Michael Lisitsa - 16 March 2026

--- 
# Outline

- Demo showing it working.
    - Cool example programs that work
    - function.lox - Call stack,
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

---

# A fully functioning language:
- Global and local variables
- Control Flow
- Functions
- Dynamically Typed
- Garbage Collector

---

# Pipeline of a language
- Scanner (Source code to tokens)
- Compiler (Tokens to Bytecode)
- Virtual Machine (Execute bytecode)

---

# Scanning

## Left-to-Right order vs. Bytecode order

<div class="columns">
<div>

```
    1 + 2 * 3 
     same as
        +
       / \
      1   *
         / \
        2   3
```


</div>
<div>

```

__OPCODE__      __VAL__
OP_CONSTANT        1
OP_CONSTANT        2
OP_CONSTANT        3
OP_MULTIPLY
OP_ADD

```

</div>
</div>

- Implemented as 1-pass-compiler, emits bytecode, no intermediate representation. 
- One-pass:   Lua, TurboPascal, C
- Multi-pass: Python, Java, C#
___What language features do Multi-pass compilers enable?___

---

# 1-pass vs Multi-pass


<div class="columns">
<div>

Python
```py

x = 10
def foo():
    print(x)
    x += 1
foo()

```

<a class="reveal red" href="https://docs.python.org/3/faq/programming.html#why-am-i-getting-an-unboundlocalerror-when-the-variable-has-a-value">
UnboundLocalError: cannot access local variable 'x' where it is not associated with a value
</a>

</div>
<div>

Lox
```lox

var x = 10;
fun foo(){
    print x;
    x = x + 1;
}
foo();

```
<p class="reveal green">10 // 1-pass compiler doesn't know about local assignment</p>

</div>
</div>

<span class="reveal"> How would you fix the python to make it work?</span>

---

# Bytecode Interpreter

<div class="columns">
<div>

```js
1 {
2   var a = 1;
3   {
4     print 2+a;
5   }
6 }
```

</div>
<div>

```
BYTE   LN OPCODE            ARG VAL
0000    2 OP_CONSTANT         0 '1'
0002    4 OP_CONSTANT         1 '2'
0004    | OP_GET_LOCAL        1
0006    | OP_ADD
0007    | OP_PRINT
0008    6 OP_POP
0009    7 OP_RETURN
```

</div>
</div>

---

# What is a Value?

<div class="columns">
<div>

```c
// 16 bytes — same size for every type
typedef struct {
  ValueType type;
  union {
    bool   boolean;
    double number;
    Obj*   obj;
  } as;
} Value;
```

</div>
<div>

```
VAL_NUMBER │ type=NUM  │  3.14        │
VAL_BOOL   │ type=BOOL │  true        │
VAL_NIL    │ type=NIL  │  -           │
VAL_OBJ    │ type=OBJ  │  ptr ──►heap │
```

Numbers and booleans stored **inline**

Strings, functions → heap pointer (`Obj*`)

</div>
</div>

> Python stores **everything** as a heap object — even small integers
> Lox numbers are bare C `double` values: no boxing, no allocation

---

# Hash Table vs Value Stack

<div class="columns">
<div>

**Hash Table** _(lookup by name string)_
```
globals:
  "score"  → 42.0
  "name"   → Obj* → "Alice"
  "greet"  → Obj* → <fn greet>

interned strings:
  "hello"  → Obj* → "hello"
  "world"  → Obj* → "world"
```
Top-level variables, functions,
and every string literal (interned once)

</div>
<div>

**Value Stack** _(lookup by slot index)_
```
slot 0: <script fn>
slot 1: 42.0        ← var a = 42
slot 2: Obj* → "hi" ← var s = "hi"
slot 3: (temp expr)
```
Local variables, temporaries,
function args — **no name lookup needed**

</div>
</div>

**Interned strings:** `"hello" == "hello"` is a pointer compare — O(1)

Every string literal is stored once in the hash table; duplicates share the same `Obj*`

---

# Call Frame

Each function has its **own bytecode + constants pool**. The value stack is one flat array.

<div class="columns">
<div>

```lox
fun double(n) { return n * 2; }
fun calc(x) {
  return double(x) + 1;
}
calc(5);
```

Value stack while `double(5)` runs:
```
[0] <script>   ← frame 0 base
[1] calc       ← frame 1 base
[2]  5           x  (calc's arg)
[3] double     ← frame 2 base
[4]  5           n  (double's arg)
```

Each call starts with a **function pointer**
then its **arguments** on the stack

</div>
<div>

```c
typedef struct {
  ObjFunction* function;
  // ↑ has its OWN chunk:
  //   bytecode + constants array
  uint8_t* ip;    // current instruction
  Value*   slots; // → into stack
} CallFrame;
```

```
frame 0  slots=stack[0]  ip→script code
frame 1  slots=stack[1]  ip→calc code
frame 2  slots=stack[3]  ip→double code
```

`OP_GET_LOCAL 1` in `double`
  reads `slots[1]` = `stack[4]` = `n`

`OP_CONSTANT 0` in `double`
  reads `double`'s `constants[0]`
  — completely separate from `calc`'s!

</div>
</div>

---

# Conditionals: Jump If False

<div class="columns">
<div>

```lox
if (score > 90) {
  print "pass";
} else {
  print "fail";
}
```

![if-else jump diagram](https://craftinginterpreters.com/image/jumping-back-and-forth/if-else.png)

</div>
<div>

```
0000 OP_GET_GLOBAL    'score'
0002 OP_CONSTANT      90
0004 OP_GREATER
0005 OP_JUMP_IF_FALSE  ──► 0015
0007 OP_POP
0008 OP_CONSTANT      'pass'
0010 OP_PRINT
0011 OP_JUMP           ──► 0019
0013 OP_POP            ◄── else branch
0014 OP_CONSTANT      'fail'
0016 OP_PRINT
```

</div>
</div>

- `OP_JUMP_IF_FALSE`: peek stack top, jump **forward** if falsy
- `OP_JUMP`: unconditional forward jump to skip the else branch
- Python's `dis` shows the same: `POP_JUMP_IF_FALSE`

---

# Loops: Jumping Backwards

<div class="columns">
<div>

```lox
var i = 0;
while (i < 3) {
  print i;
  i = i + 1;
}
```

`OP_JUMP` always jumps **forward** ↓

`OP_LOOP` always jumps **backward** ↩

`for` loops compile to the same
`OP_LOOP` instruction — just
syntactic sugar over `while`

> Python's `for` over a range compiles
> to `GET_ITER` + `FOR_ITER` —
> same backward-jump pattern

</div>
<div>

```
0000 OP_CONSTANT      0
0002 OP_DEFINE_GLOBAL 'i'
     ┌──────────────────── loop_start
0004 │ OP_GET_GLOBAL  'i'
0006 │ OP_CONSTANT    3
0008 │ OP_LESS
0009 │ OP_JUMP_IF_FALSE ──► 0025
0011 │ OP_POP
0012 │ OP_GET_GLOBAL  'i'
0014 │ OP_PRINT
0015 │ OP_GET_GLOBAL  'i'
0017 │ OP_CONSTANT    1
0019 │ OP_ADD
0020 │ OP_SET_GLOBAL  'i'
0022 │ OP_POP
0023 └─ OP_LOOP ◄──────────
0025 OP_POP
```

</div>
</div>

---

# Dynamic Typing

<div class="columns">
<div>

```c
// Every operation checks types at runtime
case OP_ADD: {
  if (IS_STRING(peek(0)) &&
      IS_STRING(peek(1))) {
    concatenate();       // string cat
  } else if (IS_NUMBER(peek(0)) &&
             IS_NUMBER(peek(1))) {
    double b = AS_NUMBER(pop());
    double a = AS_NUMBER(pop());
    push(NUMBER_VAL(a + b));
  } else {
    runtimeError("type mismatch");
  }
}
```

</div>
<div>

```lox
print 1 + 2;       // 3
print "a" + "b";   // "ab"
print 1 + "b";     // runtime error!
```

`IS_OBJ(val)` → follow pointer
→ read `obj->type` (cache miss risk)

Like Python's duck typing —
but with explicit type tags
instead of `__add__` method lookup

No compile-time type guarantees:
errors surface at runtime, just
like Python's `TypeError`

</div>
</div>

---

# Garbage Collector: Mark & Sweep

<div class="columns">
<div>

All heap objects form an intrusive linked list:

```c
struct Obj {
  ObjType type;
  bool    isMarked;
  Obj*    next;    // ← linked list
};
```

Every `malloc`'d object is chained
into `vm.objects` at allocation time

</div>
<div>

**Mark phase** — start from roots:
- Value stack (locals, temporaries)
- Hash table (globals, interned strings)

Follow every `Obj*` pointer recursively,
set `isMarked = true` on each

**Sweep phase:**
Walk the `vm.objects` linked list,
`free()` any object still unmarked,
reset `isMarked` on survivors

</div>
</div>

- Python uses **reference counting** first — objects freed instantly when count → 0
- Python's `gc` module adds mark-and-sweep **only for reference cycles**
- Lox skips reference counting entirely — simpler code, but GC pauses

---

# Compiling vs Interpreting

```
Source ──► Scanner ──► Compiler ──► ObjFunction { chunk: bytecode + constants }
                                              │
                                      vm.interpret()
                                              │
                                          run() loop  ← while(true) switch(READ_BYTE())
                                              │
                              OP_CALL ──► new CallFrame  (no recompile!)
                                              │
                                     function already has its bytecode
```

- All bytecode is compiled **before `run()` starts**
- `OP_CALL` at runtime just creates a new `CallFrame` pointing at pre-compiled bytecode
- Like Python's `.pyc` files — but compiled on-the-fly, never saved to disk

---

# Interesting Projects

**[Math.js](https://mathjs.org/)** — expression evaluator in JS

Compiles math expressions to an AST, then to a JS closure.
Uses the JS engine's own call stack and closures as its "VM".

**[Nearley](https://omrelli.ug/nearley-playground/)** — generic parser in JS

Define a grammar in BNF-like syntax, get a parser back.
Try the browser playground — watch tokens get parsed in real time.

---

# Example: Parsing
1+2*3
Depth 1: precedence = ASSIGNMENT
1. Consume 1 (prefixRule -> number).
2. Consume + (InfixRule -> binary)
Depth 2: precedence = TERM + 1 == FACTOR
3. Consume 2 (prefixRule -> number)
4. Consume * (infixRule -> binary)
Depth 3: precendence = FACTOR + 1 == UNARY
5. Consume 3 (prefixRule -> number)
While loop finishes. Since the next token is less precedence PREC_NONE.

