# LLVM Instruction Disassembly Analysis

## Overview
The code block you're asking about is **custom LLVM IR inspection code** that dumps DXIL shader instructions to a text file. It's located in `dxil_spirv_c.cpp` within the `dxil_spv_converter_run()` function.

---

## How It's Called

### Call Chain:
```
User's Application (C API)
    ↓
dxil_spv_converter_run(converter)  [dxil_spirv_c.cpp:1004]
    ↓
INSTRUCTION DUMP BLOCK [lines 799-930]
    ↓
File: "dxil_instructions_dump.txt"
```

### Context:
The code runs **AFTER** the entry point has been converted but **BEFORE** the CFG structurizer runs:

```cpp
// In dxil_spv_converter_run():

auto entry_point = dxil_converter.convert_entry_point();  // Conversion happens first

if (entry_point.entry.entry == nullptr) {
    return DXIL_SPV_ERROR_GENERIC;
}

// --- INSTRUCTION DUMP CODE RUNS HERE --- (lines 799-930)
// This is where the custom LLVM IR is written

// Then CFG structurization happens:
dxil_spv::CFGStructurizer structurizer(...);  // After the dump
```

---

## Code Breakdown

### What It Does:

1. **Opens File for Writing**
   ```cpp
   FILE *f = fopen("dxil_instructions_dump.txt", "w");
   ```
   - Output file location: **Current working directory** (usually project root)

2. **Gets the LLVM Module**
   ```cpp
   auto &llvm_module = converter->bc_parser.get_module();
   ```
   - `converter->bc_parser` is of type `LLVMBCParser`
   - `get_module()` returns a reference to the parsed LLVM module
   - This module contains all functions, basic blocks, and instructions from the DXIL bitcode

3. **Iterates Through Functions**
   ```cpp
   for (auto *func : llvm_module)
   {
       // func is a pointer to LLVMBC::Function
   }
   ```
   - Iterates using the module's iterator (`Module::begin()` / `Module::end()`)
   - `Module` is from `bc/module.hpp` (custom LLVM implementation)
   - NOT standard LLVM (it's a custom LLVM bitcode parser)

4. **Iterates Through Basic Blocks**
   ```cpp
   for (auto &bb : *func)
   {
       // bb is a BasicBlock&
   }
   ```
   - Uses `Function` iterator to get basic blocks
   - `BasicBlock` is from `bc/function.hpp`

5. **Iterates Through Instructions**
   ```cpp
   for (auto *val : bb)
   {
       // val is a LLVMBC::Value*
   }
   ```
   - Gets each instruction in the basic block
   - `Value` is base class from `bc/value.hpp`

6. **Gets Instruction Metadata**
   ```cpp
   ValueKind kind = val->get_value_kind();
   uint64_t my_id = val->get_tween_id();
   ```
   - `ValueKind` is an enum from `bc/value.hpp` identifying instruction type
   - `tween_id` is a custom ID assigned during parsing

7. **Maps ValueKind to Opcode Names**
   ```cpp
   switch(kind) {
       case ValueKind::Return: opName = "Ret"; break;
       case ValueKind::Branch: opName = "Br"; break;
       // ... etc
   }
   ```
   - Maps internal enum values to human-readable instruction names

8. **Processes Operands**
   ```cpp
   auto *inst = static_cast<LLVMBC::Instruction*>(val);
   unsigned numOps = inst->getNumOperands();
   ```
   - Casts `Value*` to `Instruction*` (all values in a block are instructions)
   - Iterates through each instruction's operands
   - Handles special cases:
     - **ConstantInt**: Prints the integer value
     - **ConstantFP**: Prints the float value
     - **Argument**: Prints as "arg#"
     - **Other**: Prints as "value_id"

---

## Where Things Are Defined

### Key Classes/Types:

| Component | Defined In | Purpose |
|-----------|-----------|---------|
| `LLVMBCParser` | `llvm_bitcode_parser.hpp` + `.cpp` | Parses LLVM bitcode into custom LLVM IR |
| `Module` | `bc/module.hpp` + `bc/module.cpp` | Custom LLVM Module representation |
| `Function` | `bc/function.hpp` + `bc/function.cpp` | Custom LLVM Function representation |
| `BasicBlock` | `bc/function.hpp` + `bc/function.cpp` | Basic block container |
| `Instruction` | `bc/instruction.hpp` + `bc/instruction.cpp` | Base class for all instructions |
| `Value` | `bc/value.hpp` + `bc/value.cpp` | Base class for all values (instructions, constants, arguments) |
| `ValueKind` | `bc/value.hpp` | Enum of instruction types |

### Hierarchy:
```
Value (base class)
├── Instruction
│   ├── BinaryOperator (Add, Sub, Mul, etc.)
│   ├── LoadInst
│   ├── StoreInst
│   ├── CallInst
│   └── ... (many others)
├── Constant
├── Argument
└── BasicBlock
```

---

## Custom vs Standard LLVM

### Why Custom LLVM?
```cpp
#ifdef HAVE_LLVMBC
    #include "bc/module.hpp"  // Custom
#else
    #include <llvm/IR/Module.h>  // Standard LLVM
#endif
```

The project includes a **custom LLVM bitcode parser** (`HAVE_LLVMBC`) that:
- Is more lightweight than full LLVM
- Only parses what's needed (bitcode → simplified IR)
- Provides `bc/` directory classes:
  - `module.cpp` (1847 lines) - Module parsing
  - `instruction.hpp/cpp` - Instruction types
  - `function.hpp/cpp` - Function/BasicBlock
  - `value.hpp/cpp` - Value types

### Fallback
If `HAVE_LLVMBC` is not defined, it falls back to standard LLVM IR from `<llvm/IR/...>`

---

## Iteration Mechanism

### Module Iterator (`for auto *func : llvm_module`)
```cpp
// In bc/module.hpp:
Vector<Function *>::const_iterator Module::begin() const;
Vector<Function *>::const_iterator Module::end() const;
```

### Function Iterator (`for auto &bb : *func`)
```cpp
// In bc/function.hpp:
IteratorAdaptor<BasicBlock, Vector<BasicBlock *>::const_iterator> Function::begin() const;
```

### BasicBlock Iterator (`for auto *val : bb`)
```cpp
// In bc/function.hpp:
IteratorAdaptor<Instruction, Vector<Instruction *>::const_iterator> BasicBlock::begin() const;
```

All use custom iterators defined in `bc/iterator.hpp` (IteratorAdaptor pattern)

---

## Operand Types Handled

The code specifically handles:

1. **ConstantInt** - Immediate integer values
2. **ConstantFP** - Immediate floating-point values
3. **Argument** - Function parameters
4. **Other** - References to other instructions/values via their ID

### Operand Access:
```cpp
inst->getNumOperands()      // Number of operands
inst->getOperand(i)          // Get operand at index i
operand->get_value_kind()   // What type of value is it
```

---

## Output Format Example

The file `dxil_instructions_dump.txt` would look like:
```
=== Shader: main ===

--- Function ---

<Block>
  %1: Alloca ...
  %2: Load %1
  %3: Add %2 5
  %4: Store %3 %1
  %5: Br %6

<Block>
  %6: Return %3
```

Format: `%<ID>: <OpcodeName> <operands>`

---

## Related Files

### Main Implementation:
- `dxil_spirv_c.cpp` - Contains the dump code (line 799-930)
- `bc/module.cpp` - Module iteration
- `bc/function.cpp` - Function/BasicBlock implementation
- `bc/instruction.hpp/cpp` - Instruction class

### LLVM Bitcode Parsing:
- `llvm_bitcode_parser.hpp/cpp` - Main parser interface
- `bc/module.cpp` (VERY LARGE - 2400+ lines) - Parses LLVM bitcode format
- `bc/` directory - Custom LLVM IR classes

### DXILConversion:
- `dxil_converter.hpp/cpp` - Main converter
- Various `opcodes/` files - Instruction handlers

---

## Key Points

✅ **Custom, not standard LLVM**
- Uses `LLVMBC::*` classes from `bc/` directory
- Lightweight bitcode parser

✅ **Called during shader conversion**
- Runs in `dxil_spv_converter_run()`
- After entry point conversion, before CFG structurization

✅ **Outputs to file**
- File: `dxil_instructions_dump.txt`
- Location: Current working directory

✅ **Iterates 3 levels deep**
- Module → Functions → BasicBlocks → Instructions

✅ **Handles different operand types**
- Constants (int/float)
- Arguments  
- Value references

---

## Debug/Analysis Uses

This code is useful for:
- **Debugging shader conversion**
- **Analyzing DXIL IR structure**
- **Understanding instruction flow**
- **Verifying operand dependencies**
- **Tracing value origins**

The `%<ID>` numbering allows you to trace how values flow through the program!
