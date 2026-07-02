#include "tac_parser/three_address_code_parser.h"
#include "tac_to_2ac/converter.h" // Added converter header
#include <iostream>
#include <string>

int main() {
    std::string input_filepath = "src/test/test.txt";
    
    std::cout << "--- Compiler Backend: Code Generation Process ---\n";
    std::cout << "Loading 3-Address Code from: " << input_filepath << "\n\n";

    rm_forge::ThreeAddressCodeParser parser(input_filepath);
    
    if (parser.parse()) {
        std::cout << "Successfully parsed 3-Address Code:\n";
        const auto& instructions = parser.get_instructions();
        for (const auto& instr : instructions) {
            instr.print();
        }
        
        std::cout << "\n--- Phase 1: Instruction Selection (TAC to 2AC) ---\n";
        
        // 1. Instantiate the converter with the parsed TAC instructions
        rm_forge::TacTo2acConverter converter(instructions);
        
        // 2. Run the conversion algorithm
        converter.convert();
        
        // 3. Retrieve the generated 2-address instructions vector
        const std::vector<rm_forge::TwoAddressInstruction>& two_addr_instructions = converter.get_2ac_instructions();
        
        std::cout << "Successfully converted to 2-Address Code. (Generated " 
                  << two_addr_instructions.size() << " instructions)\n";
                  
        std::cout << "\n--- 2-Address Code Output ---\n";
        for (const auto& instr : two_addr_instructions) {
            instr.print();
        }
        
    } else {
        std::cerr << "Failed to parse input file.\n";
        return 1;
    }

    return 0;
}
