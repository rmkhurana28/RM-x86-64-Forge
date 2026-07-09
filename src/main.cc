#include "tac_parser/three_address_code_parser.h"
#include "tac_to_2ac/converter.h" // Added converter header
#include "liveness_analysis/cfg.h"
#include <iostream>
#include <string>
#include <cstdio>

int main() {
    // Ensure output directory exists before generating files
    system("mkdir -p output");

    std::string input_filepath = "src/test/test.txt";
    
    std::cout << "--- Compiler Backend: Code Generation Process ---\n";
    std::cout << "Loading 3-Address Code from: " << input_filepath << "\n\n";

    rm_forge::ThreeAddressCodeParser parser(input_filepath);
    
    if (parser.parse()) {
        freopen("output/01_3-addr_code.txt", "w", stdout);
        std::cout << "Successfully parsed 3-Address Code:\n";
        const auto& instructions = parser.get_instructions();
        for (const auto& instr : instructions) {
            instr.print();
        }
        
        freopen("output/02_2-addr_code.txt", "w", stdout);
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
        
        // ---------------------------------------------------------
        // Phase 2: Liveness Analysis (CFG Construction)
        // ---------------------------------------------------------
        freopen("output/03_cfg.txt", "w", stdout);
        rm_forge::ControlFlowGraph cfg;
        cfg.buildCFG(two_addr_instructions);
        
        // Run Liveness Equations
        cfg.computeLiveness();
        
        // Print the newly constructed CFG and blocks
        cfg.print();
        
        // Flatten and print optimized 2AC Code for comparison
        std::vector<rm_forge::TwoAddressInstruction> final_code = cfg.getOptimizedInstructions();
        freopen("output/04_optimized_2-addr_code.txt", "w", stdout);
        std::cout << "\n--- Optimized 2-Address Code Output ---\n";
        int seq_counter = 1;
        for (const auto& instr : final_code) {
            instr.printWithSeq(seq_counter++);
        }
        
        // Ensure all output is fully written to the disk before combining
        std::cout.flush();
        std::fflush(stdout);

        // Concatenate all 4 files into a master 00 file
        system("cat output/01_3-addr_code.txt output/02_2-addr_code.txt output/03_cfg.txt output/04_optimized_2-addr_code.txt > output/00_all_in_one.txt");
        
    } else {
        std::cerr << "Failed to parse input file.\n";
        return 1;
    }

    return 0;
}
