# MOS and mstd

## Overview
M OS is an experimental AArch64 operating system written in C++ and assembly. It is built from the ground up: hardware is discovered through the device tree, physical memory is managed independently from virtual memory, page tables are constructed explicitly, and the MMU and exception machinery are established before entering the permanent kernel environment.

The project is also an experiment in using high-level C++ abstractions without surrendering low-level control. The mstd library provides the abstractions used by the kernel, while the kernel itself remains responsible for the architectural mechanisms underneath them.

## Getting Started

You need GNU Make, the **AArch64 bare-metal** GNU toolchain
(`aarch64-none-elf-as`, `aarch64-none-elf-g++`, and
`aarch64-none-elf-objcopy`), and `qemu-system-aarch64`.
The 32-bit `arm-none-eabi` toolchain cannot build this kernel.

- **macOS (Homebrew):** Install [Xcode Command Line Tools](https://developer.apple.com/documentation/xcode/installing-the-command-line-tools) for `make` (`xcode-select --install` if needed), then run `brew install --cask gcc-aarch64-embedded` and `brew install qemu`.
- **Linux:** Install GNU Make and QEMU from your distribution, then download Arm's **aarch64-none-elf** toolchain from [Arm GNU Toolchain](https://developer.arm.com/tools-and-software/gnu-toolchain#Downloads). Add its `bin` directory to `PATH`.
- **Windows:** Use GNU Make in Git Bash or MSYS2 (the recipes use `mkdir -p` and `rm -rf`). Install the Windows **aarch64-none-elf** toolchain from [Arm GNU Toolchain](https://developer.arm.com/tools-and-software/gnu-toolchain#Downloads) and [QEMU](https://www.qemu.org/download/), and add their binaries to `PATH`.

Check that the tools are available:

```sh
aarch64-none-elf-as --version
aarch64-none-elf-g++ --version
qemu-system-aarch64 --version
```

Then build from the repository root:

```sh
make
```

The kernel artifacts are written to `build/kernel.elf` and
`build/kernel.img`.

To boot the kernel in QEMU:

```sh
qemu-system-aarch64 -M virt -cpu cortex-a53 -m 512M -nographic -kernel build/kernel.elf
```

Exit QEMU with `Ctrl-A`, then `X`.

To remove generated files:

```sh
make clean
```

### Why C++
We like C and C++ as much as most of us like Rust. But C++ is far better than C for our purposes: it provides better abstractions such as templates and concepts, powerful features such as RAII and lambdas, useful references and strong type composition, and less boilerplate than C.\
I remember how grateful I was when C++ programming gave me nice, clean, and shorter code compared to spending hours writing the same thing in C. \
While Rust is good and fits our philosophy well, we didn't choose Rust for the following reasons.\
First, we initially didn't want to fight the borrow checker, which is ironic since it turned out to be one of the best things we could have had.\
Second, the combination of high-level abstractions and low-level control in safe Rust initially felt somewhat like an oxymoron. Not because Rust couldn't do it, but because we didn't want to rely heavily on unsafe, where it can be tempting to simply bypass the compiler's checks when things become difficult.\
It turns out that we are deliberately establishing safe and unsafe boundaries much like Rust does, just without the compiler validating them for us. We eventually learned that manually doing Rust-like safety in C++ is a lot like manually doing things in C that C++ can automatically do.
Finally, we just wanted to try something new with C++.

## M Operating System

## Experimental Library : mstd
mstd is a style-over-substance, experimental library in which we integrate some interesting parts of Rust and Zig philosophy into C++.\
The idea started when we decided to program a bare-metal system in C++, where most runtime-dependent features of C++, such as exceptions and the heap, are disabled. This prompted us to ask: how can we make a portable, highly abstract library that works in both the kernel and userspace, while also being different from what already exists?\
And so, mstd was born.

### Freestanding First
The library does not depend on libc and is designed to function as the foundational library of M OS. When running in a conventional hosted environment, it can interoperate nicely with libc rather than requiring it.

### No RTTI
If runtime polymorphism is needed, it should be explicitly represented by the abstraction that requires it. However, after hours of consideration on downcasting, we decided to implement downcasting mechanism in a very limited way, while discouraging it.

### Mutable by Default
C++ is fundamentally a language where objects are mutable and operations may have side effects, which does not naturally align with Rust philosophy. So we choose to accept it, not fighting it. That's where mstd differs from Rust philosophy.
Types are mutable by default, while immutability is introduced when it is useful or required, primarily through const.

### Prefer Static over Dynamic
While runtime polymorphism is allowed, our philosophy is to lean towards compile-time polymorphism whenever practical. We therefore prefer things such as CRTP, concepts, and a bit of typestate, and hope for the best from compiler optimization. Runtime polymorphism isn't forbidden. We simply consider it avoidable until proven necessary.

The following are ~~shamelessly stolen~~inspired by other languages.

### Monadic Types

As big fans of Rust and functional programming languages, we (or only I) believe monadic types make for a rather nice way to express control flow and error handling. This also fits nicely with our environment, where exceptions are not available.\
So, instead of throwing exceptions and hoping for the best, we pass our problems around in little wrappers.

`maybe<T>` : represents an optional value. It can contain either `some<T>` or `nothing`.Unlike `std::optional`, `maybe<T>` is designed around monadic operations and is intended to work with values, references, and firm types.

`result<T,E>`: represents either a successful value of type `T` or an error of type `E`. It supports monadic operations. The goal is to make error propagation explicit and composable without requiring exceptions.

`firm<T, Validation>` : represents a value with a guaranteed invariant. Instead of merely representing "a value of type T", a firm type represents "a value of type T that is known to satisfy some condition." This allows invariants to be established once and then relied upon by the rest of the program. `maybe<firm>` is also niche-optimized.

### Traits and Duck Typing

What is the best way to enable runtime polymorphism without using intrusive vtables like ordinary C++ virtual functions?\
Suppose we have an interface. How can we make that interface work both at compile time and at runtime?\
Here comes our Rust-like trait design pattern. The idea is to use C++ concepts at compile time, and a dynamic trait wrapper at runtime. A dynamic trait object is essentially a fat pointer: it stores a pointer to a vtable containing type-erased functions, along with a pointer to the type-erased object itself. \ 
This is where the library sits somewhere between Go and Rust. As long as a type satisfies the required concept, it can be type-erased and assigned to the corresponding trait object.\
We also allow limited downcasting in this design pattern where vtable  serves as the identity of a type. Downcasting is generally frowned upon so we decided to make it just a secondary escape hatch that only works locally to a linkage unit.\ 
With this approach, runtime polymorphism works without requiring objects to inherit from a common base class. There is also no intrusive metadata stored inside the objects themselves.

### Sum Type

Like Rust's enum, we provide a type for storing heterogeneous data called choice. We aim for choice to eventually replace `std::variant` in our environment for several reasons:

- `choice` is specifically designed to work with objects and abstractions provided by this library.
- `choice` supports both O(N) lookup and O(1) dispatch, depending on how it is used.
- `choice` provides cleaner functional-ish syntax that works well with our style.
- Its interface is designed around the rest of the library rather than around the constraints of the C++ standard library.

### Explicit Allocators

While the library provides a global general-purpose allocator, memory management should still be transparent. Every allocation and deallocation requires an explicit allocator. Consequently, all container types take an allocator as part of their memory-management interface. We don't want allocations to magically happen somewhere behind the curtains. If memory is being acquired, we would like you to know about it.

### Some Useful Features
#### Lazy Evaluation
mstd provides abstractions for lazy evaluation using lambdas and RAII. This is useful for deferring computation until its result is actually needed. Of course, this becomes considerably less clever when the deferred computation has side effects.

#### Defer
mstd provides defferable cleanup through scope guards which are implemented on top of RAII.

### Limitations
Unfortunately, C++ does not come with a borrow checker, so we leave lifetime management to the user.\
In particular, the library won't save you from things like use-after-move, or allowing a heap-allocated object to outlive the heap it came from. We provide the abstractions; you provide the discipline.\
C++'s syntax also does not have built-in support for propagating errors through our `maybe<T>` and `result<T, E>` types. As a result, some operations have to be done manually.\
We could try to invent increasingly elaborate macros to fix this, but we hate macros, end of the story.
