# instruction.hpp Analysis

## Overview
This file defines the **custom LLVM Instruction class hierarchy** used by the DXIL-SPIRV project. It's the backbone of how instructions are represented in the custom LLVM IR.

---

## Class Hierarchy

```
Instruction (base class)
├── ReturnInst
├── UnreachableInst
├── CallInst
├── UnaryOperator
├── BinaryOperator
├── CastInst
├── SelectInst
├── ExtractValueInst
├── AllocaInst
├── GetElementPtrInst
├── LoadInst
├── StoreInst
├── CmpInst
│   ├── FCmpInst (floating-point compare)
│   └── ICmpInst (integer compare)
├── BranchInst
├── SwitchInst
├── PHINode
├── AtomicRMWInst
├── AtomicCmpXchgInst
├── ShuffleVectorInst
├── ExtractElementInst
├── InsertElementInst
└── CompositeConstructInst
```

---

## Base Instruction Class

### Key Members:
```cpp
class Instruction : public Value
{
    // Core methods
    bool isTerminator() const;
    Value *getOperand(unsigned index) const;
    unsigned getNumOperands() const;
    
    // Metadata/Debug info
    MDNode *getMetadata(const String &str) const;
    bool hasMetadata(const String &str) const;
    void setMetadata(const String &str, MDNode *node);
    
    // Protected for derived classes
    Vector<Value *> operands;           // Instruction arguments
    UnorderedMap<String, MDNode *> attachments;  // Debug metadata
    bool is_terminator;                 // Marks control flow terminals
};
```

### Key Enums in Instruction:
```cpp
enum Predicate {
    // Floating-point comparisons
    FCMP_FALSE, FCMP_OEQ, FCMP_OGT, FCMP_OGE, FCMP_OLT, FCMP_OLE, 
    FCMP_ONE, FCMP_ORD, FCMP_UNO, FCMP_UEQ, FCMP_UGT, FCMP_UGE, 
    FCMP_ULT, FCMP_ULE, FCMP_UNE, FCMP_TRUE,
    
    // Integer comparisons
    ICMP_EQ, ICMP_NE, ICMP_UGT, ICMP_UGE, ICMP_ULT, ICMP_ULE,
    ICMP_SGT, ICMP_SGE, ICMP_SLT, ICMP_SLE
};

enum CastOps {
    Trunc, ZExt, SExt, FPToUI, FPToSI, UIToFP, SIToFP,
    FPTrunc, FPExt, PtrToInt, IntToPtr, BitCast, AddrSpaceCast
};

enum BinaryOps {
    Add, FAdd, Sub, FSub, Mul, FMul, UDiv, SDiv, FDiv,
    URem, SRem, FRem, Shl, LShr, AShr, And, Or, Xor
};
```

---

## Individual Instruction Classes

### **Control Flow Instructions**

#### ReturnInst
```cpp
class ReturnInst : public Instruction
{
    ReturnInst(Value *value);          // value can be nullptr
    Value *getReturnValue() const;     // Get return value (or nullptr)
};
```
**Used for:** Function return statements

#### BranchInst
```cpp
class BranchInst : public Instruction
{
    // Unconditional
    BranchInst(BasicBlock *true_block);
    
    // Conditional
    BranchInst(BasicBlock *true_block, BasicBlock *false_block, Value *cond);
    
    bool isConditional() const;
    Value *getCondition() const;       // Only if conditional
    unsigned getNumSuccessors() const;
    BasicBlock *getSuccessor(unsigned index) const;
};
```
**Used for:** If/else control flow

#### SwitchInst
```cpp
class SwitchInst : public Instruction
{
    SwitchInst(Value *cond, BasicBlock *default_block, unsigned num_cases);
    void addCase(Value *case_value, BasicBlock *bb);
    
    Value *getCondition() const;
    BasicBlock *getDefaultDest() const;
    Vector<Case>::const_iterator case_begin() const;
    Vector<Case>::const_iterator case_end() const;
    
    struct Case {
        Value *value;              // Case value
        BasicBlock *bb;            // Jump target
    };
};
```
**Used for:** Switch statements

#### UnreachableInst
```cpp
class UnreachableInst : public Instruction
{
    UnreachableInst();
};
```
**Used for:** Unreachable code paths

---

### **Memory Instructions**

#### LoadInst
```cpp
class LoadInst : public Instruction
{
    LoadInst(Type *type, Value *ptr);
    Value *getPointerOperand() const;
};
```
**Format:** `%result = load type, ptr %pointer`

#### StoreInst
```cpp
class StoreInst : public Instruction
{
    StoreInst(Value *ptr, Value *value);
};
```
**Format:** `store type %value, ptr %pointer`

#### AllocaInst
```cpp
class AllocaInst : public Instruction
{
    AllocaInst(Type *pointer_type, Type *element_type, Value *size);
    Value *getArraySize() const;
};
```
**Format:** `%ptr = alloca type, size`

#### GetElementPtrInst
```cpp
class GetElementPtrInst : public Instruction
{
    GetElementPtrInst(Type *pointer_type, Vector<Value *> arguments, bool inbounds);
    bool isInBounds() const;
};
```
**Format:** `%ptr = getelementptr [inbounds] base_type, ptr %base, indices...`

---

### **Arithmetic Instructions**

#### BinaryOperator
```cpp
class BinaryOperator : public Instruction
{
    BinaryOperator(Value *LHS, Value *RHS, BinaryOps op);
    BinaryOps getOpcode() const;
    
    void setFast(bool enabled);        // Fast math flag
    bool isFast() const;
};

enum BinaryOps {
    Add, FAdd, Sub, FSub, Mul, FMul, UDiv, SDiv, FDiv,
    URem, SRem, FRem, Shl, LShr, AShr, And, Or, Xor
};
```
**Examples:** `%result = add i32 %a, %b`, `%result = fmul fast float %a, %b`

#### UnaryOperator
```cpp
class UnaryOperator : public Instruction
{
    UnaryOperator(UnaryOps uop, Value *value);
    UnaryOps getOpcode() const;
    
    enum class UnaryOps { Invalid, FNeg, INeg };
};
```
**Examples:** `%result = fneg float %a`

#### CastInst
```cpp
class CastInst : public Instruction
{
    CastInst(Type *type, Value *value, Instruction::CastOps op);
    Instruction::CastOps getOpcode() const;
};

enum CastOps {
    Trunc, ZExt, SExt, FPToUI, FPToSI, UIToFP, SIToFP,
    FPTrunc, FPExt, PtrToInt, IntToPtr, BitCast, AddrSpaceCast
};
```
**Examples:** `%result = zext i32 %a to i64`, `%result = bitcast float %a to i32`

---

### **Comparison Instructions**

#### CmpInst (Base)
```cpp
class CmpInst : public Instruction
{
    CmpInst(ValueKind kind, Predicate pred, Value *LHS, Value *RHS);
    Predicate getPredicate() const;
    
    enum Predicate { FCMP_*, ICMP_* };
};
```

#### FCmpInst
```cpp
class FCmpInst : public CmpInst
{
    FCmpInst(Predicate pred, Value *LHS, Value *RHS);
};
```
**Example:** `%result = fcmp oeq float %a, %b` → true if `a == b` (ordered)

#### ICmpInst
```cpp
class ICmpInst : public CmpInst
{
    ICmpInst(Predicate pred, Value *LHS, Value *RHS);
};
```
**Example:** `%result = icmp slt i32 %a, %b` → true if `a < b` (signed)

---

### **Vector Instructions**

#### ShuffleVectorInst
```cpp
class ShuffleVectorInst : public Instruction
{
    ShuffleVectorInst(Type *type, Value *a, Value *b, Value *shuf);
    int getMaskValue(unsigned index) const;
};
```
**Example:** `%result = shufflevector <4 x i32> %v1, <4 x i32> %v2, <4 x i32> <i32 0, i32 1, i32 4, i32 5>`

#### ExtractElementInst
```cpp
class ExtractElementInst : public Instruction
{
    ExtractElementInst(Value *vec, Value *offset);
    Value *getVectorOperand() const;
    Value *getIndexOperand() const;
};
```
**Example:** `%e = extractelement <4 x i32> %vec, i32 2` → Gets element at index 2

#### InsertElementInst
```cpp
class InsertElementInst : public Instruction
{
    InsertElementInst(Value *vec, Value *value, Value *index);
};
```
**Example:** `%result = insertelement <4 x i32> %vec, i32 %e, i32 2`

#### CompositeConstructInst (Custom Extension)
```cpp
class CompositeConstructInst : public Instruction
{
    CompositeConstructInst(Type *type, Vector<Value *> constituents);
};
```
**Custom LLVM extension** - Constructs composite types (not in standard LLVM)

---

### **Aggregate Instructions**

#### ExtractValueInst
```cpp
class ExtractValueInst : public Instruction
{
    ExtractValueInst(Type *type, Value *aggregate, Vector<unsigned> indices);
    Value *getAggregateOperand() const;
    unsigned getNumIndices() const;
    const unsigned *getIndices() const;
};
```
**Example:** `%e = extractvalue {i32, float} %struct, 1` → Gets float field

#### SelectInst
```cpp
class SelectInst : public Instruction
{
    SelectInst(Value *true_value, Value *false_value, Value *cond);
};
```
**Example:** `%result = select i1 %cond, i32 %true_val, i32 %false_val` → Ternary operator

---

### **Function Call**

#### CallInst
```cpp
class CallInst : public Instruction
{
    CallInst(FunctionType *function_type, Function *callee, Vector<Value *> params);
    Function *getCalledFunction() const;
};
```
**Example:** `%result = call i32 @func(i32 %a, i32 %b)`

---

### **PHI Node (Special)**

#### PHINode
```cpp
class PHINode : public Instruction
{
    PHINode(Type *type, size_t num_edges);
    unsigned getNumIncomingValues() const;
    Value *getIncomingValue(unsigned index) const;
    BasicBlock *getIncomingBlock(unsigned index) const;
    void add_incoming(Value *value, BasicBlock *bb);
    
    struct Incoming {
        Value *value;
        BasicBlock *bb;
    };
};
```
**Purpose:** Merges values from multiple basic blocks
**Example:**
```
%phi = phi i32 [ %a, %block1 ], [ %b, %block2 ]
```
Selects `%a` if coming from `%block1`, or `%b` if coming from `%block2`

---

### **Atomic Instructions**

#### AtomicRMWInst
```cpp
class AtomicRMWInst : public Instruction
{
    AtomicRMWInst(Type *type, Value *ptr, Value *value, BinOp op);
    Value *getPointerOperand() const;
    Value *getValOperand() const;
    BinOp getOperation() const;
    
    enum class BinOp {
        Xchg, Add, Sub, And, Nand, Or, Xor, Max, Min, UMax, UMin, FAdd, FSub
    };
};
```
**Example:** `%old = atomicrmw add ptr %addr, i32 1 seq_cst`

#### AtomicCmpXchgInst
```cpp
class AtomicCmpXchgInst : public Instruction
{
    AtomicCmpXchgInst(Value *ptr, Value *cmp, Value *new_value, Type *type_override = nullptr);
    Value *getPointerOperand() const;
    Value *getNewValOperand() const;
    Value *getCompareOperand() const;
};
```
**Example:** `%result = cmpxchg ptr %addr, i32 %cmp, i32 %new seq_cst seq_cst`

---

## Macro Helper

```cpp
#define LLVMBC_DEFAULT_VALUE_KIND_IMPL
// Provides default implementations for get_value_kind() 
// and RTTI-like dispatch
```

Used by ALL instruction subclasses to implement proper type identification.

---

## Connection to the Dump Code

In the instruction dump code we saw earlier:

```cpp
ValueKind kind = val->get_value_kind();  // Returns one of these ValueKind values

switch(kind) {
    case ValueKind::Return: opName = "Ret"; break;
    case ValueKind::Load: opName = "Load"; break;
    case ValueKind::Store: opName = "Store"; break;
    case ValueKind::BinaryOperator: opName = "BinaryOp"; break;
    // ... etc
}
```

**Each class in instruction.hpp defines its own `ValueKind`**:
- `ReturnInst::get_value_kind()` → `ValueKind::Return`
- `LoadInst::get_value_kind()` → `ValueKind::Load`
- `BinaryOperator::get_value_kind()` → `ValueKind::BinaryOperator`
- etc.

This is how the dump code identifies what type of instruction it's looking at!

---

## Key Design Patterns

1. **Inheritance-based Type System**
   - All instructions inherit from `Instruction`
   - Each has a `get_value_kind()` method for RTTI

2. **Operand Storage**
   - Base `Instruction` class stores `operands` vector
   - Subclasses use `getOperand()` to access them
   - Methods like `getPointerOperand()`, `getReturnValue()` provide semantic access

3. **Metadata Support**
   - Debug information attached via `UnorderedMap<String, MDNode *>`
   - Used for source-level debugging and analysis

4. **Terminator Marking**
   - Control-flow instructions mark themselves as terminators
   - `isTerminator()` identifies branch, return, switch, etc.

5. **Custom Extensions**
   - `CompositeConstructInst` added for SPIR-V mapping
   - `UnaryOperator::INeg` for integer negation

---

## File Statistics

- **577 lines total**
- **23 instruction classes** defined
- **Pure C++ header** - all implementations in `instruction.cpp`
- Compiles to `llvm-bc` library (from `bc/CMakeLists.txt`)

This is the **core abstraction layer** for working with LLVM instructions in this project!
