# Initialize cmake

```bash
mkdir build && cd build
cmake .. -D CMAKE_C_COMPILER=gcc
make
```

# Debugging Neovim
Place file in examples/main.lox
```c
expr -- (TokenType)(parser.current.type) // Get current token being processed e.g. TOKEN_EQUAL
expr -- (Precedence)(precedence) // Get precedence e.g. PREC_ASSIGNMENT
// All current function bytecodes. Disassembler will have nicer names 
parray 9 current->function->chunk.code 

```

# Bytecodes

   0 OP_CONSTANT [1]     10 OP_SET_GLOBAL [1]    20 OP_PRINT
   1 OP_NIL               11 OP_EQUAL             21 OP_JUMP [2]
   2 OP_TRUE              12 OP_GREATER           22 OP_JUMP_IF_FALSE [2]
   3 OP_FALSE             13 OP_LESS              23 OP_LOOP [2]
   4 OP_CALL [1]          14 OP_ADD               24 OP_RETURN
   5 OP_POP               15 OP_SUBTRACT
   6 OP_GET_LOCAL [1]     16 OP_MULTIPLY
   7 OP_SET_LOCAL [1]     17 OP_DIVIDE
   8 OP_GET_GLOBAL [1]    18 OP_NOT
   9 OP_DEFINE_GLOBAL [1] 19 OP_NEGATE

# Entrypoints
## Scanner: 
Inited from [advance-#L82](~/Documents/lear/clang/crafting-interpreters-in-c/compiler.c|82). 
Approx stack frame is:
	scanToken scanner.c:191
	advance compiler.c:85
	match compiler.c:108
	statement compiler.c:534
	declaration compiler.c:517
	compile compiler.c:778
	interpret vm.c:110

