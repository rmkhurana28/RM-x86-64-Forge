#ifndef RM_FORGE_TAC_TO_2AC_CONVERTER_H
#define RM_FORGE_TAC_TO_2AC_CONVERTER_H

#include <vector>
#include <string>
#include "../tac_parser/instruction.h"
#include "two_address_instruction.h"

namespace rm_forge {

class TacTo2acConverter {
public:
    // Constructor accepts the parsed 3-address instructions
    explicit TacTo2acConverter(const std::vector<Instruction>& tac_instructions);

    // Method to execute the conversion logic
    void convert();

    // Returns the generated 2-address instructions
    const std::vector<TwoAddressInstruction>& get_2ac_instructions() const;

    std::string* getNewTempRegister();

    // Helper function to easily push new 2-address instructions
    void emit(const std::string& opcode, const std::string& operand1, const std::string& operand2 = "");
    
    
    void handleAssignment(Instruction* current);
    void handleReturn(Instruction* current);
    void handleBinary(Instruction* current);
    void handleLabel(Instruction* current);
    void handleUnconditionalGoto(Instruction* current);
    unsigned short handleParam(vector<Instruction> current , int index);

private:
    long long tempRegisterCount = 0; // Moved from global scope to fix linker error
    std::vector<Instruction> tac_instructions_;
    std::vector<TwoAddressInstruction> two_addr_instructions_;
};

} // namespace rm_forge

#endif // RM_FORGE_TAC_TO_2AC_CONVERTER_H
