#include "cfg.h"
#include <iostream>

namespace rm_forge {

ControlFlowGraph::ControlFlowGraph() {
}

std::shared_ptr<BasicBlock> ControlFlowGraph::createNewBlock() {
    auto block = std::make_shared<BasicBlock>(next_block_id++);
    basic_blocks.push_back(block);
    return block;
}

bool ControlFlowGraph::isUnconditionalJump(const std::string& opcode) const {
    return opcode == "JMP";
}

bool ControlFlowGraph::isConditionalJump(const std::string& opcode) const {
    return opcode == "JE" || opcode == "JNE" || opcode == "JL" || 
           opcode == "JLE" || opcode == "JG" || opcode == "JGE" || 
           opcode == "JZ" || opcode == "JNZ";
}

bool ControlFlowGraph::isReturn(const std::string& opcode) const {
    return opcode == "ret" || opcode == "RET";
}

bool ControlFlowGraph::isLastInstruction(size_t index, size_t total_size) const {
    return index == total_size - 1;
}

bool ControlFlowGraph::isLabel(const std::string& opcode) const {
    // Labels in your 2AC are emitted as the opcode itself (e.g. "L1", "main").
    // We can assume it is a label if it isn't a known x86 instruction.
    // A quick heuristic: if it starts with 'L' (and isn't LEA) or is a function name.
    return (opcode.find("L") == 0 && opcode != "LEA") || opcode == "main" || opcode.find("f") == 0;
}

void ControlFlowGraph::buildCFG(const std::vector<TwoAddressInstruction>& instructions) {
    // Logic to identify leaders, split instructions into blocks, and create edges
    
    long unsigned i = 0;

    newBlock:

    auto currBlock = createNewBlock();
    
    createdNewBlock:
    
    while(i<instructions.size()){
        auto currInstruction = instructions[i];
        string decider = currInstruction.getOpcode();
        
        if(isUnconditionalJump(decider) || isConditionalJump(decider) || isReturn(decider)){
            // cout << "Case-1\n";
            currBlock->addInstruction(currInstruction);
            
            // basic_blocks.push_back(currBlock);

            if(!isLastInstruction(i , instructions.size())){
                i++;
                goto newBlock;
            }    
        } else if(isLabel(decider)){
            // cout << "Case-2\n";
            // basic_blocks.push_back(currBlock);
            if(currBlock->getInstructions().size() != 0){
                //
                currBlock = createNewBlock();
            }
            currBlock->addInstruction(currInstruction);
            i++;
            goto createdNewBlock;
        } else{
            // cout << "Case-3\n";
            currBlock->addInstruction(currInstruction);
        }

        i++;
    }

    // cfgOUT analysis
    for(size_t j=0 ; j<basic_blocks.size() ; j++){   
        // store current block     
        auto currBlockHelper = basic_blocks[j];

        // if it is return statement, do NOT add anything in cfgOUT
        if(isReturn(currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOpcode())){
            cout << "Return\n";
            continue;
        } 

        // if it is unconditonal statement, add the block whose 1st insturction is this label
        if(isUnconditionalJump(currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOpcode())){
            cout << "unconditional";
            // get label name
            string label = currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOperand1();
            cout << " | " << label << endl;
            
            // search through all the blocks
            for(size_t k=0 ; k<basic_blocks.size() ; k++){
                if(basic_blocks[k]->getInstructions()[0].getOpcode() == label){
                    currBlockHelper->addSuccessor(basic_blocks[k]);
                    break;
                }
            }
            continue;
        }

        if(isConditionalJump(currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOpcode())){
            cout << "conditional";
            if(j != basic_blocks.size()-1)
                currBlockHelper->addSuccessor(basic_blocks[j+1]);
            
            string label = currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOperand1();
            cout << " | " << label << endl;
            
            for(size_t k=0 ; k<basic_blocks.size() ; k++){
                if(basic_blocks[k]->getInstructions()[0].getOpcode() == label){
                    currBlockHelper->addSuccessor(basic_blocks[k]);
                    break;
                }
            }
            continue;
        }

        cout << "normal\n";

        if(j != basic_blocks.size()-1)
            currBlockHelper->addSuccessor(basic_blocks[j+1]);
    }

    // cfgIN analysis
    for(size_t j=0 ; j<basic_blocks.size() ; j++){
        auto currBlockHelper = basic_blocks[j];

        for(size_t k=0 ; k<currBlockHelper->cfg_out.size() ; k++){
            for(size_t l=0 ; l<basic_blocks.size() ; l++){
                if(currBlockHelper->cfg_out[k] == basic_blocks[l]){
                    basic_blocks[l]->addPredecessor(currBlockHelper);
                    break;
                }
            }
        }
    }

    
}

void ControlFlowGraph::computeLiveness() {
    // Logic to calculate USE/DEF per block, and iterate LIVE IN/OUT equations
}

void ControlFlowGraph::print() const {
    std::cout << "\n--- Phase 2: Control Flow Graph (CFG) ---\n";
    if (basic_blocks.empty()) {
        std::cout << "CFG is empty.\n";
        return;
    }
    for (const auto& block : basic_blocks) {
        block->print();
    }
}

} // namespace rm_forge
