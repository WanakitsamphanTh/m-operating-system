## Memory Offset
Memory is accessed with brackets `[]` similar to NASM. In AARCH64 asm, the notation differs in how the base register is update.
| syntax | description |
| - | - |
| `[x0] immediate` | access the memory x0 points to |
| `[x0, #8] immediate` | acces memory at address x0 + 8 |
| `[x0, #8]! pre-indexed` | increase x0 by 8 and access to the memory it points to |
| `[x0], #8 post-indexed` | access to memory x0 points to and add 8 to x0 |

## Stack manipulation
Instead of puhs/pop instruction, stack manipulation in AARCH64 is done as loading from and storing in the memory that the stack pointer `sp` is pointing to. Two registers can be pushed and popped simultaneously. *The order of registers pushed into the stack in the same instruction is not reversed*
```asm
str x0, [sp, -#8]!
ldr x0, [sp], #8
stp x0, x1, [sp, -#16]!
ldp x0, x1 [sp], #16
```

## Registers
| registers | usage | category |
| - | - | - |
| x0 - x7 | arguments to functions/return values | caller-saved |
| x8 | indirect result/return value address | caller-saved |
| x9 - x15 | scratch register | caller-saved |
| x16 - x17 | linker register | caller-saved |
| x18 | platform register (reserved for platform-specific use) | depends |
| x19 - x28 | general-purpose | callee-saved |
| x29 | frame pointer | callee-saved |
| x30 | link register | callee-saved |
| sp | stack pointer | callee-saved |
| xzr | always 0 | always 0 |
| pc | program counter | - |
| rp | processor state flag | - |

`x0-x30` are 64-bit registers and `w0-w30` are 32-bit registers of the same physical registers. 

## V Registers
V registers are a separate set of floating-point and SIMD registers. There are 32 registers.
| registers | usage | category |
| - | - | - |
| v0 - v7 | arguments to functions/return values | caller-saved |
| v8 - v15 | temporary registers | only lower 64 bits are preserved |
| v16 - v31 | temporary registers | caller-saved |

The same physical V register can be accessed at different size based on the prefix
| prefix | size | usage |
| - | - | - |
| vN | 128-bit | SIMD vector |
| qN | 128-bit | same as above |
| dN | 64-bit | double-precision float |
| sN | 32-bit | single-precision float |
| hN | 16-bit | half-precision float |
| bN | 8-bit | byte SIMD |

## Calling Convention
1. stack must be 16-byte aligned
2. arguments are passed in `x0-x7` registers and excessive arguments are passed via stack
3. Integer return value in `x0`. A larger return value is divided and returned via registers consecutively starting from `x0`.
4. returning struct is done via register `x8` holding the pointer
5. Single precision floating point is returned via `s0` and double precision floating point via `d0`.
