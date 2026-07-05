#ifndef RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
#define RM_FORGE_TWO_ADDRESS_INSTRUCTION_H

#include <string>

using namespace std;

namespace rm_forge {

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

    int getId() const { return id; }
    const string& getOpcode() const { return opcode; }
    const string& getOperand1() const { return operand1; }
    const string& getOperand2() const { return operand2; }

private:
    string opcode;
    string operand1;
    string operand2;
    int id;
};

} // namespace rm_forge

#endif // RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
