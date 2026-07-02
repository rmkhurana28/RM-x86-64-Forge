#include "tac_parser/three_address_code_parser.h"
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
    } else {
        std::cerr << "Failed to parse input file.\n";
        return 1;
    }

    return 0;
}
