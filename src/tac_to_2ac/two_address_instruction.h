#ifndef RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
#define RM_FORGE_TWO_ADDRESS_INSTRUCTION_H

#include <string>

using namespace std;

namespace rm_forge {

class TwoAddressInstruction {
public:
    TwoAddressInstruction(const string& op, const string& op1, const string& op2 = "") 
        : opcode(op), operand1(op1), operand2(op2) {}

    void print() const {
        static int snum = 1; // Tracks the 2AC line number across prints
        // Just a quick debug print helper
        if (operand2.empty()) {
            // e.g., PUSH RAX or IDIV v1
            printf("[%03d] %s %s\n", snum++, opcode.c_str(), operand1.c_str());
        } else {
            // e.g., MOV RAX, v1
            printf("[%03d] %s %s, %s\n", snum++, opcode.c_str(), operand1.c_str(), operand2.c_str());
        }
    }

private:
    string opcode;
    string operand1;
    string operand2;
};

} // namespace rm_forge

#endif // RM_FORGE_TWO_ADDRESS_INSTRUCTION_H
