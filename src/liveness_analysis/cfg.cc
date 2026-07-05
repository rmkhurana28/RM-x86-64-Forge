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

    // calculating def/use
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        auto currBlock = basic_blocks[i];
        
        // calculating def/use
        for(size_t j=0 ; j<currBlock->getInstructions().size() ; j++){
            string op = currBlock->getInstructions()[j].getOpcode();
            if(op == "ADD" || op == "SUB" || op == "IMUL" || op == "AND" || op == "OR" || op == "XOR"){                                              
                // 1,2 going to USE
                // 1 going to def
                
                // checking if it alr exists in def
                if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                    currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists

                // checking if it is a constant                    
                if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                    // checking if it alr exist in def
                    if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand2()) == 0)
                        currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand2()); // add if doesnt exists

                currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
            } else if(op == "SHL" || op == "SAR"){
                // 1 goes to use
                // rcx (cl) goes to use
                // 1 going to def

                // checking if it alr exists in def
                if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                    currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists

                // checking if it alr exists in def
                if(currBlock->def_set.count("rcx") == 0)
                    currBlock->use_set.insert("rcx"); // add if doesnt exists

                currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
            } 
            else if(op == "NEG" || op == "NOT"){

                // 1 going to USE
                // 1 going to def

                // checking if it alr exists in def
                if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                    currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists

                currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
            } else if(op == "IDIV"){
                // 1, rax, rdx going to USE
                // rax, rdx going to def
                
                // checking if it is a constant                    
                if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                    // checking if it alr exist in def
                    if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                        currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists

                // add rax to use if not in def
                if(currBlock->def_set.count("rax") == 0)
                    currBlock->use_set.insert("rax");

                // add rdx to use if not in def                    
                if(currBlock->def_set.count("rdx") == 0)
                    currBlock->use_set.insert("rdx");
                    
                currBlock->def_set.insert("rax");
                currBlock->def_set.insert("rdx");
            } else if(op == "CQO"){
                // rax going to use
                // rdx going to def

                // add rax to use if not in def
                if(currBlock->def_set.count("rax") == 0)
                    currBlock->use_set.insert("rax");

                currBlock->def_set.insert("rdx");                    
            } else if(op == "CMP"){
                // 1,2 going to use
                
                // checking if it is a constant                    
                if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                    // checking if it alr exist in def
                    if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                        currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists

                // checking if it is a constant                    
                if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                    // checking if it alr exist in def
                    if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand2()) == 0)
                        currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand2()); // add if doesnt exists
            } else if(op == "PUSH"){
                // 1 goes to use

                // checking if it is a constant                    
                if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                    // checking if it alr exist in def
                    if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand1()) == 0)
                        currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand1()); // add if doesnt exists
            } else if(op == "POP"){
                // 1 goes to def

                currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
            } else if(op == "MOV" || op == "LEA"){
                // 2 goes to use
                // 1 goes to def

                if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){
                    std::unordered_set<string> listOp2 = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());

                    for(const string& currVar : listOp2){
                        // check if it is const
                        if(!isConstant(currVar))
                            // checking if it alr exist in def
                            if(currBlock->def_set.count(currVar) == 0)
                                currBlock->use_set.insert(currVar); // add if doesnt exists   
                    }

                    if(is_memory_operand(currBlock->getInstructions()[j].getOperand1())){
                        std::unordered_set<string> listOp1 = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());

                        for(const string& currVar : listOp1){
                             if(!isConstant(currVar))
                                // checking if it alr exist in def
                                if(currBlock->def_set.count(currVar) == 0)
                                    currBlock->use_set.insert(currVar); // add if doesnt exists   
                        }
                    } else{
                        currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
                    }
                } else if (is_memory_operand(currBlock->getInstructions()[j].getOperand1())){
                    
                    std::unordered_set<string> listOp1 = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                    for(const string& currVar : listOp1){
                         if(!isConstant(currVar))
                            // checking if it alr exist in def
                            if(currBlock->def_set.count(currVar) == 0)
                                currBlock->use_set.insert(currVar); // add if doesnt exists   
                    }

                    
                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        // checking if it alr exist in def
                        if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand2()) == 0)
                            currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand2()); // add if doesnt exists
                }
                else{
                    // checking if it is a constant                    
                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        // checking if it alr exist in def
                        if(currBlock->def_set.count(currBlock->getInstructions()[j].getOperand2()) == 0)
                            currBlock->use_set.insert(currBlock->getInstructions()[j].getOperand2()); // add if doesnt exists                
                    
                    currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
                }


                
            } else if(op == "MOVZX"){
                // rax (al) goes to use
                // 1 goes to def

                if(currBlock->def_set.count("rax") == 0)
                    currBlock->use_set.insert("rax");

                currBlock->def_set.insert(currBlock->getInstructions()[j].getOperand1());
            }
            else if(op == "CALL"){
                // param registers goes to use, depending on value of arg2
                // rax goes to def

                vector<string> helperString;
                helperString.push_back("rdi");
                helperString.push_back("rsi");
                helperString.push_back("rdx");
                helperString.push_back("rcx");
                helperString.push_back("r8");
                helperString.push_back("r9");

                for(unsigned short k=0 ; k<stoi(currBlock->getInstructions()[j].getOperand2()) && k<6; k++){
                    if(currBlock->def_set.count(helperString[k]) == 0)
                        currBlock->use_set.insert(helperString[k]);
                }

                currBlock->def_set.insert("rax");
            } else if(op == "ret"){
                // rax goes to use

                if(currBlock->def_set.count("rax") == 0)
                    currBlock->use_set.insert("rax");
            } else if(op == "SETE" || op == "SETNE" || op == "SETL" || op == "SETLE" || op == "SETG" || op == "SETGE"){
                // rax (al) goes to def

                currBlock->def_set.insert("rax");
            } else if(op == "JMP" || op == "JE" || op == "JNE" || op == "JL" || op == "JLE" || op == "JG" || op == "JGE" || isLabel(op)) continue;
            else{
                continue;
            }
                        
        }

        currBlock->use_set.erase("rsp");
        currBlock->use_set.erase("rbp");
        currBlock->def_set.erase("rsp");
        currBlock->def_set.erase("rbp");

    }

    // now, we have def/use

    

    // calculating liveIn/liveOut

    // initalizing liveIn to use
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        basic_blocks[i]->live_in = basic_blocks[i]->use_set;    
    }
    
    bool modified = false;

    again:
    cout << "here\n";

    modified = false;

    // iteratively calculating liveIn/liveOut
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        auto currBlock = basic_blocks[i];

        // adding elements to liveIn
        for(const string& element : currBlock->live_out){
            if(currBlock->def_set.count(element) == 0){
                size_t old_size = currBlock->live_in.size();
                currBlock->live_in.insert(element);
                if(!modified && currBlock->live_in.size() > old_size) modified = true;
            }
        }

        for(size_t j=0 ; j<currBlock->cfg_out.size() ; j++){
            for(const string& element : currBlock->cfg_out[j]->live_in){   
                size_t old_size = currBlock->live_out.size();             
                currBlock->live_out.insert(element);
                
                if(!modified && currBlock->live_out.size() > old_size) modified = true;
            }
        }
    }

    if(modified) goto again;
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
