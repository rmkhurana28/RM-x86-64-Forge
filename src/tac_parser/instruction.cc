#include "instruction.h"
#include <iomanip>

namespace rm_forge {

std::string Instruction::type_to_string(InstructionType t) {
    switch (t) {
        case InstructionType::BinaryOperation: return "B"; // Binary
        case InstructionType::UnaryOperation: return "U";  // Unary
        case InstructionType::Assignment: return "=";      // Assignment
        case InstructionType::Label: return "L";           // Label
        case InstructionType::UnconditionalGoto: return "G"; // Goto
        case InstructionType::ConditionalGoto: return "?";   // Conditional Goto
        case InstructionType::Parameter: return "P";       // Parameter
        case InstructionType::FunctionCall: return "C";    // Call
        case InstructionType::FunctionCallWithReturn: return "K"; // Call with return
        case InstructionType::Return: return "R";          // Return
        case InstructionType::ArrayLoad: return "[";       // Array Load
        case InstructionType::ArrayStore: return "]";      // Array Store
        case InstructionType::AddressOf: return "&";       // Address Of
        case InstructionType::PointerLoad: return "*";     // Pointer Load
        case InstructionType::PointerStore: return "#";    // Pointer Store
        case InstructionType::Unknown:
        default: return "X";                               // Unknown
    }
}

void Instruction::print() const {
    std::cout << "[" << std::setw(3) << std::setfill('0') << snum_ << "] (" << type_to_string(type_) << ") ";
    switch (type_) {
        case InstructionType::BinaryOperation:
            std::cout << result_ << " = " << arg1_ << " " << op_ << " " << arg2_ << "\n";
            break;
        case InstructionType::UnaryOperation:
            std::cout << result_ << " = " << op_ << " " << arg1_ << "\n";
            break;
        case InstructionType::Assignment:
            std::cout << result_ << " = " << arg1_ << "\n";
            break;
        case InstructionType::Label:
            std::cout << result_ << ":\n";
            break;
        case InstructionType::UnconditionalGoto:
            std::cout << "goto " << result_ << "\n";
            break;
        case InstructionType::ConditionalGoto:
            std::cout << "if " << arg1_ << " " << op_ << " " << arg2_ << " goto " << result_ << "\n";
            break;
        case InstructionType::Parameter:
            std::cout << "param " << arg1_ << "\n";
            break;
        case InstructionType::FunctionCall:
            std::cout << "call " << arg1_ << ", " << arg2_ << "\n";
            break;
        case InstructionType::FunctionCallWithReturn:
            std::cout << result_ << " = call " << arg1_ << ", " << arg2_ << "\n";
            break;
        case InstructionType::Return:
            std::cout << "return " << arg1_ << "\n";
            break;
        case InstructionType::ArrayLoad:
            std::cout << result_ << " = " << arg1_ << "[" << arg2_ << "]\n";
            break;
        case InstructionType::ArrayStore:
            std::cout << result_ << "[" << arg1_ << "] = " << arg2_ << "\n";
            break;
        case InstructionType::AddressOf:
            std::cout << result_ << " = &" << arg1_ << "\n";
            break;
        case InstructionType::PointerLoad:
            std::cout << result_ << " = *" << arg1_ << "\n";
            break;
        case InstructionType::PointerStore:
            std::cout << "*" << result_ << " = " << arg1_ << "\n";
            break;
        default:
            std::cout << "Unknown Instruction Format\n";
            break;
    }
}

} // namespace rm_forge
