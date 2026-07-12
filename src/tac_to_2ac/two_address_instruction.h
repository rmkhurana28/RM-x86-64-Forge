#ifndef RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
#define RM_FORGE_TWO_ADDRESS_INSTRUCTION_H

#include <string>
#include <unordered_set>
#include <unordered_map>

using namespace std;

namespace rm_forge {

inline bool isConstant(const string& operand) {
    if (operand.empty()) return false;
    size_t start = 0;
    if (operand[0] == '-') {
        if (operand.length() == 1) return false;
        start = 1;
    }
    for (size_t i = start; i < operand.length(); ++i) {
        if (operand[i] < '0' || operand[i] > '9') return false;
    }
    return true;
}

inline bool isPhysicalRegister(const string& operand) {
    static const std::unordered_set<string> physical_registers = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", "rbp", "rsp",
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15",
        "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp", "esp",
        "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d",
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
        "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w",
        "al", "bl", "cl", "dl", "sil", "dil", "bpl", "spl",
        "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
        "ah", "bh", "ch", "dh"
    };
    return physical_registers.count(operand) > 0;
}

// Helper 1: Returns true if Operand 1 is modified (DEF)
inline bool isOp1Modified(const string& opcode) {
    return (opcode == "ADD" || opcode == "SUB" || opcode == "IMUL" || 
            opcode == "AND" || opcode == "OR" || opcode == "XOR" ||
            opcode == "SHL" || opcode == "SAR" || opcode == "NEG" || 
            opcode == "NOT" || opcode == "POP" || opcode == "MOV" || 
            opcode == "LEA" || opcode == "MOVZX" || 
            opcode == "SETE" || opcode == "SETNE" || opcode == "SETL" || 
            opcode == "SETLE" || opcode == "SETG" || opcode == "SETGE");
}

// Helper 2: Returns true if Operand 1 is read (USE)
inline bool isOp1Read(const string& opcode) {
    return (opcode == "ADD" || opcode == "SUB" || opcode == "IMUL" || 
            opcode == "AND" || opcode == "OR" || opcode == "XOR" ||
            opcode == "SHL" || opcode == "SAR" || opcode == "NEG" || 
            opcode == "NOT" || opcode == "IDIV" || opcode == "CMP" || 
            opcode == "PUSH");
}


// Helper 4: Returns true if Operand 2 is read (USE)
inline bool isOp2Read(const string& opcode) {
    return (opcode == "ADD" || opcode == "SUB" || opcode == "IMUL" || 
            opcode == "AND" || opcode == "OR" || opcode == "XOR" ||
            opcode == "CMP" || opcode == "MOV" || opcode == "LEA" || 
            opcode == "MOVZX" || opcode == "SHL" || opcode == "SAR");
}

// Helper 5: Returns true if the instruction is a Jump or Call (Target is a label or function)
inline bool isJumpOrCall(const string& opcode) {
    return (opcode == "JMP" || opcode == "JE" || opcode == "JNE" || 
            opcode == "JL" || opcode == "JLE" || opcode == "JG" || 
            opcode == "JGE" || opcode == "CALL");
}

inline bool is_memory_operand(const string& operand) {
    if (operand.length() < 2) return false;
    return operand.front() == '[' && operand.back() == ']';
}

inline std::unordered_set<string> extract_memory_vars(const string& operand) {
    std::unordered_set<string> vars;
    
    if (!is_memory_operand(operand)) {
        return vars;
    }

    string inner = operand.substr(1, operand.length() - 2);
    size_t plus_pos = inner.find(" + ");
    
    if (plus_pos != string::npos) {
        string v1 = inner.substr(0, plus_pos);
        vars.insert(v1);
        
        string right_side = inner.substr(plus_pos + 3); 
        size_t star_pos = right_side.find("*8");
        
        if (star_pos != string::npos) {
            string v2 = right_side.substr(0, star_pos);
            vars.insert(v2);
        } else {
            vars.insert(right_side); 
        }
    } else {
        vars.insert(inner);
    }
    
    return vars;
}

// Helper: Convert graph color ID to physical x86-64 register string
inline std::string getPhysicalRegisterName(int id) {
    static const std::string regNames[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", 
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    if (id >= 0 && id < 14) return regNames[id];
    return "UNKNOWN_REG";
}

// Helper: Reconstructs a memory operand string with physical registers
inline std::string map_memory_operand(const std::string& op, const std::unordered_map<std::string, int>& varRegMap) {
    if (!is_memory_operand(op)) return op;
    
    std::string in = op.substr(1, op.length() - 2);
    size_t p = in.find(" + ");
    
    // Tiny lambda to map a single variable string if it exists
    auto mapVar = [&](std::string v) {
        if (!isConstant(v) && !isPhysicalRegister(v) && v != "" && varRegMap.count(v) > 0) {
            return getPhysicalRegisterName(varRegMap.at(v));
        }
        return v;
    };

    // Reconstruct the string perfectly
    if (p != std::string::npos) {
        std::string v1 = in.substr(0, p);
        std::string v2 = in.substr(p + 3, in.find("*8") - p - 3);
        return "[" + mapVar(v1) + " + " + mapVar(v2) + "*8]";
    }
    
    return "[" + mapVar(in) + "]";
}

class TwoAddressInstruction {
public:
    TwoAddressInstruction(const string& op, const string& op1, const string& op2 = "") 
        : opcode(op), operand1(op1), operand2(op2) {
        static int global_id = 0;
        id = ++global_id;
    }

    void print() const {
        // Just a quick debug print helper
        if (operand2.empty()) {
            // e.g., PUSH RAX or IDIV v1
            printf("[%03d] %s %s\n", id, opcode.c_str(), operand1.c_str());
        } else {
            // e.g., MOV RAX, v1
            printf("[%03d] %s %s, %s\n", id, opcode.c_str(), operand1.c_str(), operand2.c_str());
        }
    }

    void printWithSeq(int seq) const {
        if (operand2.empty()) {
            printf("[%03d] <- [%03d] %s %s\n", seq, id, opcode.c_str(), operand1.c_str());
        } else {
            printf("[%03d] <- [%03d] %s %s, %s\n", seq, id, opcode.c_str(), operand1.c_str(), operand2.c_str());
        }
    }

    int getId() const { return id; }
    void setId(int new_id) { id = new_id; }
    const string& getOpcode() const { return opcode; }
    const string& getOperand1() const { return operand1; }
    const string& getOperand2() const { return operand2; }
    
    void setOperand1(const string& op1) { operand1 = op1; }
    void setOperand2(const string& op2) { operand2 = op2; }

private:
    string opcode;
    string operand1;
    string operand2;
    int id;
};

// Global helper to easily create a new instruction on the fly
inline TwoAddressInstruction makeInstruction(const string& op, const string& op1, const string& op2 = "") {
    return TwoAddressInstruction(op, op1, op2);
}

} // namespace rm_forge

#endif // RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
