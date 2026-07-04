#ifndef RM_FORGE_INSTRUCTION_H
#define RM_FORGE_INSTRUCTION_H

#include <string>
#include <iostream>

namespace rm_forge {

enum class InstructionType {
    BinaryOperation, // done
    // e.g., a = b + c, a = b - c, *, /, %, ==, !=, <, <=, >, >=, &&, ||, &, |, ^, <<, >>

    UnaryOperation, // done
    // e.g., a = -b, a = !b, a = ~b

    Assignment, // done
    // e.g., a = b

    Label, // done
    // e.g., L1:

    UnconditionalGoto, // done
    // e.g., goto L1

    ConditionalGoto,
    // e.g., if a goto L1, ifFalse a goto L1

    Parameter, // done
    // e.g., param x (Pushing arguments before a call)

    FunctionCall, // done
    // e.g., call foo, 2 (Call a function without capturing a return value)

    FunctionCallWithReturn, // done
    // e.g., a = call foo, 2 (Call a function and save the return value)

    Return, // done
    // e.g., return a, return

    ArrayLoad, // done
    // e.g., a = b[i]

    ArrayStore, // done
    // e.g., a[i] = b

    AddressOf, // done
    // e.g., a = &b

    PointerLoad, // done
    // e.g., a = *b (Dereference right side)

    PointerStore, // done
    // e.g., *a = b (Dereference left side)

    Unknown
    // Fallback for unparseable instructions
};

class Instruction {
public:
    Instruction() : snum_(0), type_(InstructionType::Unknown) {}

    void set_snum(int s) { snum_ = s; }
    int get_snum() const { return snum_; }

    void set_type(InstructionType t) { type_ = t; }
    InstructionType get_type() const { return type_; }

    void set_result(const std::string& r) { result_ = r; }
    const std::string& get_result() const { return result_; }

    void set_arg1(const std::string& a) { arg1_ = a; }
    const std::string& get_arg1() const { return arg1_; }

    void set_op(const std::string& o) { op_ = o; }
    const std::string& get_op() const { return op_; }

    void set_arg2(const std::string& a) { arg2_ = a; }
    const std::string& get_arg2() const { return arg2_; }

    void print() const;
    static std::string type_to_string(InstructionType t);

private:
    int snum_;
    InstructionType type_;
    std::string result_;
    std::string arg1_;
    std::string op_;
    std::string arg2_;
};

} // namespace rm_forge

#endif // RM_FORGE_INSTRUCTION_H
