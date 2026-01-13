# yo

A bytecode compiler and stack-based VM for a statically-typed language.

Inspired by Inferno OS's Limbo language and Dis virtual machine.

## Features

- Static type system: `int`, `bool`, `struct`, `array`, `slice`, `pointer`
- Go/Limbo-style syntax: `fn`, `:=`, `for`, `if`
- Recursion, slicing, struct literals
- Module system with `package`, `import`, `export`

## Build

	make

## Run

Compile a single package:

	./com ./test/fib
	./vm/run obj

## Test

Run the full test with package imports:

	./com ./test/fib && cp exported obj ./test/fib/
	./com ./test/nqueen && cp exported obj ./test/nqueen/
	./com ./test && ./vm/run obj
	# result 147

## Example

```
// test/fib/fib.yo
package fib

export fn fib(n int) int {
    if n <= 1 { return n; };
    return fib(n-1) + fib(n-2);
};
```

```
// test/main.yo
package main

import "./test/fib"
import "./test/nqueen"

fn main() int {
    return fib.fib(10) + nqueen.solve(8);
};
```

See [test/](test/) for full examples including N-Queens solver with slices.

## Architecture

Five-stage compilation pipeline:

- [lex.c](lex.c): tokenizer
- [parse.c](parse.c): recursive descent parser, builds AST
- [assert.c](assert.c), [type.c](type.c): semantic analysis, type checking
- [gen.c](gen.c), [com.c](com.c): instruction selection, register allocation
- [dis.c](dis.c): bytecode serialization

VM execution:

- [vm/vm.c](vm/vm.c): stack-based VM with activation frames
- [vm/dec.c](vm/dec.c): bytecode decoder

## Files

	lex.c       lexical analyzer
	parse.c     syntax parser
	node.c      AST node construction
	type.c      type system
	decl.c      declaration and scope management
	gen.c       instruction generation
	com.c       expression compiler
	asm.c       assembly output
	dis.c       bytecode emitter
	vm/         virtual machine

## References

- https://github.com/inferno-os/inferno-os/tree/master/dis
- https://github.com/inferno-os/inferno-os/tree/master/limbo
