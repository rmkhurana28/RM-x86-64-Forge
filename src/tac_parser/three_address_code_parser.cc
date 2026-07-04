#include "three_address_code_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <regex>

namespace rm_forge {

ThreeAddressCodeParser::ThreeAddressCodeParser(const std::string& filepath) 
    : filepath_(filepath) {}

bool ThreeAddressCodeParser::parse() {
    std::ifstream file(filepath_);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filepath_ << "\n";
        return false;
    }

    std::string line;
    int current_snum = 1;
    while (std::getline(file, line)) {
        // Remove leading/trailing whitespaces (basic trim)
        line.erase(0, line.find_first_not_of(" \t\r\n"));
        line.erase(line.find_last_not_of(" \t\r\n") + 1);

        // Skip empty lines and comments
        if (line.empty() || (line.size() >= 2 && line.substr(0, 2) == "//")) {
            continue;
        }

        Instruction instr = parse_line(line);
        instr.set_snum(current_snum++);
        instructions_.push_back(instr);
    }

    return true;
}

const std::vector<Instruction>& ThreeAddressCodeParser::get_instructions() const {
    return instructions_;
}

Instruction ThreeAddressCodeParser::parse_line(const std::string& line) const {
    Instruction instr;
    std::smatch match;

    // We use a set of regular expressions to determine the instruction type
    // and extract arguments. Note: Order matters!

    // 1. Label: L:
    std::regex label_re(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\s*:$)");
    if (std::regex_match(line, match, label_re)) {
        instr.set_type(InstructionType::Label);
        instr.set_result(match[1]);
        return instr;
    }

    // 2a. Conditional Goto (Binary): if/ifFalse x relop y goto L
    std::regex cond_goto_bin_re(R"(^(if|ifFalse)\s+(.+?)\s+(==|!=|<|>|<=|>=)\s+(.+?)\s+goto\s+([a-zA-Z_][a-zA-Z0-9_]*)$)");
    if (std::regex_match(line, match, cond_goto_bin_re)) {
        if (match[1].str() == "if") {
            instr.set_type(InstructionType::ConditionalGoto);
        } else {
            instr.set_type(InstructionType::ConditionalGotoIfFalse);
        }
        instr.set_arg1(match[2]);
        instr.set_op(match[3]);
        instr.set_arg2(match[4]);
        instr.set_result(match[5]);
        return instr;
    }

    // 2b. Conditional Goto (Single): if/ifFalse x goto L
    std::regex cond_goto_single_re(R"(^(if|ifFalse)\s+(.+?)\s+goto\s+([a-zA-Z_][a-zA-Z0-9_]*)$)");
    if (std::regex_match(line, match, cond_goto_single_re)) {
        if (match[1].str() == "if") {
            instr.set_type(InstructionType::ConditionalGotoSingle);
        } else {
            instr.set_type(InstructionType::ConditionalGotoSingleIfFalse);
        }
        instr.set_arg1(match[2]);
        instr.set_result(match[3]);
        return instr;
    }

    // 3. Unconditional Goto: goto L
    std::regex goto_re(R"(^goto\s+([a-zA-Z_][a-zA-Z0-9_]*)$)");
    if (std::regex_match(line, match, goto_re)) {
        instr.set_type(InstructionType::UnconditionalGoto);
        instr.set_result(match[1]);
        return instr;
    }

    // 4. Function Call with Return: x = call f, n
    std::regex call_ret_re(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*call\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(\d+)$)");
    if (std::regex_match(line, match, call_ret_re)) {
        instr.set_type(InstructionType::FunctionCallWithReturn);
        instr.set_result(match[1]);
        instr.set_arg1(match[2]);
        instr.set_arg2(match[3]);
        return instr;
    }

    // 5. Function Call: call f, n
    std::regex call_re(R"(^call\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*,\s*(\d+)$)");
    if (std::regex_match(line, match, call_re)) {
        instr.set_type(InstructionType::FunctionCall);
        instr.set_arg1(match[1]);
        instr.set_arg2(match[2]);
        return instr;
    }

    // 6. Parameter: param x
    std::regex param_re(R"(^param\s+(.+)$)");
    if (std::regex_match(line, match, param_re)) {
        instr.set_type(InstructionType::Parameter);
        instr.set_arg1(match[1]);
        return instr;
    }

    // 7. Return: return x
    std::regex return_re(R"(^return\s*(.*)$)"); // return can have no arg
    if (std::regex_match(line, match, return_re)) {
        instr.set_type(InstructionType::Return);
        instr.set_arg1(match[1]);
        return instr;
    }

    // 8. Array Store: a[i] = x
    std::regex arr_store_re(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\[(.+?)\]\s*=\s*(.+)$)");
    if (std::regex_match(line, match, arr_store_re)) {
        instr.set_type(InstructionType::ArrayStore);
        instr.set_result(match[1]); // array name
        instr.set_arg1(match[2]);   // index
        instr.set_arg2(match[3]);   // value
        return instr;
    }

    // 9. Pointer Store: *p = x
    std::regex ptr_store_re(R"(^\*\s*([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(.+)$)");
    if (std::regex_match(line, match, ptr_store_re)) {
        instr.set_type(InstructionType::PointerStore);
        instr.set_result(match[1]); // pointer
        instr.set_arg1(match[2]);   // value
        return instr;
    }

    // Assignments
    std::regex assign_re(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\s*=\s*(.+)$)");
    if (std::regex_match(line, match, assign_re)) {
        instr.set_result(match[1]);
        std::string rhs = match[2];
        std::smatch rhs_match;

        // a) Array Load: a[i]
        std::regex arr_load_re(R"(^([a-zA-Z_][a-zA-Z0-9_]*)\[(.+?)\]$)");
        if (std::regex_match(rhs, rhs_match, arr_load_re)) {
            instr.set_type(InstructionType::ArrayLoad);
            instr.set_arg1(rhs_match[1]); // array name
            instr.set_arg2(rhs_match[2]); // index
            return instr;
        }

        // b) Address Of: &y
        std::regex addr_of_re(R"(^\&\s*([a-zA-Z_][a-zA-Z0-9_]*)$)");
        if (std::regex_match(rhs, rhs_match, addr_of_re)) {
            instr.set_type(InstructionType::AddressOf);
            instr.set_arg1(rhs_match[1]);
            return instr;
        }

        // c) Pointer Load: *p
        std::regex ptr_load_re(R"(^\*\s*([a-zA-Z_][a-zA-Z0-9_]*)$)");
        if (std::regex_match(rhs, rhs_match, ptr_load_re)) {
            instr.set_type(InstructionType::PointerLoad);
            instr.set_arg1(rhs_match[1]);
            return instr;
        }

        // d) Unary Operation: op y (e.g. -y, ~y)
        std::regex unary_op_re(R"(^([\-\!~])\s*(.+)$)");
        if (std::regex_match(rhs, rhs_match, unary_op_re)) {
            instr.set_type(InstructionType::UnaryOperation);
            instr.set_op(rhs_match[1]);
            instr.set_arg1(rhs_match[2]);
            return instr;
        }

        // e) Binary Operation: y op z
        std::regex bin_op_re(R"(^(.+?)\s+(==|!=|<=|>=|<|>|&&|\|\||<<|>>|[\+\-\*/%&\|\^])\s+(.+)$)");
        if (std::regex_match(rhs, rhs_match, bin_op_re)) {
            instr.set_type(InstructionType::BinaryOperation);
            instr.set_arg1(rhs_match[1]);
            instr.set_op(rhs_match[2]);
            instr.set_arg2(rhs_match[3]);
            return instr;
        }

        // f) Standard Assignment: y
        instr.set_type(InstructionType::Assignment);
        instr.set_arg1(rhs);
        return instr;
    }

    instr.set_type(InstructionType::Unknown);
    return instr;
}

} // namespace rm_forge
