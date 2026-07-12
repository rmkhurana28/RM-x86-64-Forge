#include "cfg.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <unordered_map>

typedef struct{
    int nodeId; // unique nodeId for each node
    string name; // name containing real register or var name
    short color; // -1 for uncolor, 0-13 for physical registers
    int degree; // number of edges
    int spillCost; // cost to spill this node
    bool isPhysical; // is this node a physical register node
    bool inGraph; // is it in the graph or in the stack

    unordered_set<int> neigh; // set of id(s) of neighboring nodes
} Node;

namespace rm_forge {

void drawInterferenceEdges(const string& defVar, 
                           const unordered_set<string>& currentLive, 
                           vector<Node>& myGraph, 
                           unordered_map<string, int>& stringToIdMap) {
                               
    // 1. Safety Check: Skip if the defined variable is a constant or a stack pointer
    if (isConstant(defVar) || is_stack_pointer(defVar)) return;

    // 2. Find or create the Node for the DEFINED variable
    int defId = -1;
    if (stringToIdMap.count(defVar) != 0) {
        defId = stringToIdMap[defVar];
    } else {
        Node temp;
        temp.color = -1;
        temp.degree = 0;
        temp.inGraph = true;
        temp.isPhysical = false;
        temp.name = defVar;
        temp.nodeId = myGraph.size();
        temp.spillCost = -1;
        
        defId = temp.nodeId;
        stringToIdMap[defVar] = defId;
        myGraph.push_back(temp);
    }

    // 3. Draw edges against the CURRENT LIVE set
    for (const string& currLiveString : currentLive) {
        
        // Safety Check: Skip constants and stack pointers in the live set
        if (isConstant(currLiveString) || is_stack_pointer(currLiveString)) continue;

        // Find or create the Node for the LIVE variable
        int useId = -1;
        if (stringToIdMap.count(currLiveString) == 0) {
            Node temp;
            temp.color = -1;
            temp.degree = 0;
            temp.inGraph = true;
            temp.isPhysical = false;
            temp.name = currLiveString;
            temp.nodeId = myGraph.size();
            temp.spillCost = -1;
            
            useId = temp.nodeId;
            stringToIdMap[currLiveString] = useId;
            myGraph.push_back(temp);
        } else {
            useId = stringToIdMap[currLiveString];
        }
        
        // Safety Check: Do NOT draw an edge to itself
        if (defId == useId) continue;
        
        // Safety Check: If the edge already exists, skip it
        if (myGraph[defId].neigh.count(useId) != 0) continue;
        
        // Draw the undirected edge and increment degrees
        myGraph[defId].neigh.insert(useId);
        myGraph[defId].degree++;
        
        myGraph[useId].neigh.insert(defId);
        myGraph[useId].degree++;
    }
}

void printInterferenceGraph(const vector<Node>& myGraph) {
    std::ofstream out("output/05_interference_graph.txt");
    out << "\n--- Phase 3: Interference Graph ---\n";
    for (const auto& node : myGraph) {
        if (!node.inGraph) continue;
        out << "Node: " << node.name 
            << " (ID: " << node.nodeId 
            << ", Physical: " << (node.isPhysical ? "Yes" : "No")
            << ", Degree: " << (node.degree == (int) INTMAX_MAX ? "INF" : std::to_string(node.degree)) 
            << ")\n  Interferes With: ";
        if (node.neigh.empty()) {
            out << "None";
        } else {
            for (int adjId : node.neigh) {
                out << myGraph[adjId].name << " ";
            }
        }
        out << "\n\n";
    }
    out.close();
}

void printPhase4Stack(const vector<Node>& myGraph, const vector<int>& helperStack) {
    // 1. Append remaining graph to 05_interference_graph.txt
    std::ofstream out5("output/05_interference_graph.txt", std::ios::app);
    out5 << "\n--- Graph After Simplify (Should only be Physical Registers) ---\n";
    for(size_t i=0 ; i<myGraph.size() ; i++){
        if(myGraph[i].inGraph){
            out5 << "Node: " << myGraph[i].name 
                 << " (ID: " << myGraph[i].nodeId 
                 << ", Physical: " << (myGraph[i].isPhysical ? "Yes" : "No")
                 << ", Degree: " << (myGraph[i].degree == (int) INTMAX_MAX ? "INF" : std::to_string(myGraph[i].degree)) 
                 << ")\n";
        }
    }
    out5.close();

    // 2. Print the stack to 06_stack.txt
    std::ofstream out6("output/06_stack.txt");
    out6 << "\n--- Phase 4: Stack (Pop Order / Top to Bottom) ---\n";
    for(int i = helperStack.size() - 1; i >= 0; i--){
        int nId = helperStack[i];
        out6 << "Stack Level [" << i << "]: Node " << myGraph[nId].name << " (ID: " << nId << ")\n";
    }
    out6.close();
}

void printPhase4MappingAndSpills(const unordered_map<string, int>& varRegMap, const vector<int>& spillNeeded, const vector<Node>& myGraph, const unordered_map<string, string>& varFuncMap) {
    const char* regNames[14] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", 
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };

    std::string outputContent = "\n--- Phase 4: Register Mapping and Spills ---\n";
    outputContent += "Mapping (Var -> Physical Color ID):\n";
    if (varRegMap.empty()) {
        outputContent += "  None\n";
    } else {
        int mapCount = 1;
        for (const auto& pair : varRegMap) {
            std::string regName = (pair.second >= 0 && pair.second < 14) ? regNames[pair.second] : "UNKNOWN";
            char buf[256];
            snprintf(buf, sizeof(buf), "  [%03d] %s -> %d (%s)\n", mapCount++, pair.first.c_str(), pair.second, regName.c_str());
            outputContent += buf;
        }
    }
    
    outputContent += "\nSpill Needed Stack (Node IDs):\n";
    if (spillNeeded.empty()) {
        outputContent += "  None\n";
    } else {
        int spillCount = 1;
        for (size_t i = 0; i < spillNeeded.size(); ++i) {
            int nId = spillNeeded[i];
            char buf[256];
            snprintf(buf, sizeof(buf), "  [%03d] %s (ID: %d)\n", spillCount++, myGraph[nId].name.c_str(), nId);
            outputContent += buf;
        }
    }
    
    outputContent += "\nVariable to Function Context Map:\n";
    if (varFuncMap.empty()) {
        outputContent += "  None\n";
    } else {
        int vCount = 1;
        for (const auto& pair : varFuncMap) {
            char buf[256];
            snprintf(buf, sizeof(buf), "  [%03d] %s -> %s\n", vCount++, pair.first.c_str(), pair.second.c_str());
            outputContent += buf;
        }
    }
    
    std::ofstream out7("output/07_mapping_and_spills.txt");
    out7 << outputContent;
    out7.close();
    
    std::ofstream out0("output/00_all_in_one.txt", std::ios::app);
    out0 << outputContent;
    out0.close();
}

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
        } else if(isLabel(decider) || decider == "FUNC"){
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
            continue;
        } 

        // if it is unconditonal statement, add the block whose 1st insturction is this label
        if(isUnconditionalJump(currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOpcode())){
            // get label name
            string label = currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOperand1();
            
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
            if(j != basic_blocks.size()-1)
                currBlockHelper->addSuccessor(basic_blocks[j+1]);
            
            string label = currBlockHelper->getInstructions()[currBlockHelper->getInstructions().size()-1].getOperand1();
            
            for(size_t k=0 ; k<basic_blocks.size() ; k++){
                if(basic_blocks[k]->getInstructions()[0].getOpcode() == label){
                    currBlockHelper->addSuccessor(basic_blocks[k]);
                    break;
                }
            }
            continue;
        }


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

    unordered_map<string , string> varFuncMap;
    unordered_map<string , int> funcOffsetMap;
    unordered_map<string , int> funcCalleeOffsetMap;
    unordered_map<string, unordered_map<string, string>> calleeSavedMap;


    runLiveAnalysisAgain:

    vector<int> deleteVector;
    vector<TwoAddressInstruction> optInstructions;
    vector<TwoAddressInstruction> modifiedInstructions;

    
    

    evaluateAgain:

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
                // rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11 goes to def (caller-saved)

                // caller saved registered
                currBlock->def_set.insert("rax");
                currBlock->def_set.insert("rcx");
                currBlock->def_set.insert("rdx");
                currBlock->def_set.insert("rsi");
                currBlock->def_set.insert("rdi");
                currBlock->def_set.insert("r8");
                currBlock->def_set.insert("r9");
                currBlock->def_set.insert("r10");
                currBlock->def_set.insert("r11");
                

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

    // now liveIn and liveOut sets are also calculated and finalized

    // proceeding to dce now
    
    // performing dce now
    for(size_t i=0 ; i<basic_blocks.size(); i++){
        auto currBlock = basic_blocks[i];

        // copy liveOut of currBlock to currentLive
        unordered_set<string> currentLive = currBlock->live_out;

        // go through all instruction of currBlock in reverse order
        for(int j=currBlock->getInstructions().size()-1 ; j>=0 ; j--){
            string op = currBlock->getInstructions()[j].getOpcode();

            if(op == "MOV" || op == "LEA" || op == "PUSH" || op == "POP" || op == "CMP" || op == "CALL" || op == "ret"){
                // never delete instructions
                // MOV is never delete if dest is memory operand

                // ading lea here coz forgot to implement lea, since it has same logic as mov, i added it here with mov

                // it reads rax
                if(op == "ret") 
                    currentLive.insert("rax");
                
                // it reads op1 and op2 and updates RFLAGS(not counted as write btw)
                else if(op == "CMP"){
                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                                currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                } 

                // it reads op1 and pushes it's value to stack (rsp/rbp are NOT coutned as write in this phase)
                else if(op == "PUSH") {
                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                // it writes to op1 and updates the rsp/rbp
                } else if(op == "POP") 
                    // if op1 is present in currentLive, remove it
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());
                
                // write to rax, and reads from registers containing the param values
                else if(op == "CALL"){

                    // remove caller-saved registers from currentLive
                    currentLive.erase("rax");
                    currentLive.erase("rcx");
                    currentLive.erase("rdx");
                    currentLive.erase("rsi");
                    currentLive.erase("rdi");
                    currentLive.erase("r8");
                    currentLive.erase("r9");
                    currentLive.erase("r10");
                    currentLive.erase("r11");                    

                    // remove rax from currentLive 
                    currentLive.erase("rax");
                    
                    vector<string> paramReg;
                    paramReg.push_back("rdi");
                    paramReg.push_back("rsi");
                    paramReg.push_back("rdx");
                    paramReg.push_back("rcx");
                    paramReg.push_back("r8");
                    paramReg.push_back("r9");

                    // insert param registers to currentLive depending on the number of params
                    for(unsigned short k=0 ; k<stoi(currBlock->getInstructions()[j].getOperand2()) && k<6 ; k++){
                        currentLive.insert(paramReg[k]);
                    }

                    
                } else if(op == "MOV" || op == "LEA"){
                    if(is_memory_operand(currBlock->getInstructions()[j].getOperand1())){
                        // write part is memory operand, hence this instruction can never be deleted

                        if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){
                            // write and read, both are memory operand

                            // extract vars from write memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }

                            // extract vars from read memory part and insert in currentLive
                            holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }
                            
                        } else{
                            // only write is memory operand, read is register or var or const

                            // extract vars from write memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }

                            // insert read part to currentLive
                            if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                                if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))                                
                                    currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                        }
                    } else{
                        // wrtie part is NOT memory operand, hence it can be deleted if possible

                        // if write part(op1) is NOT in currentLive, then this instruction can be safely deleted
                        if(currentLive.count(currBlock->getInstructions()[j].getOperand1()) == 0){
                            if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                                deleteVector.push_back(currBlock->getInstructions()[j].getId());
                                continue;                        
                            }
                        }
                        
                        // now the write(op1) is in the currentLive

                        // remove op1 from currentLive
                        currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                        if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){
                            // write is NOT memory operand
                            // read is memory operand

                            // extract vars from read memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }
                        } else{
                            // neither of write or read is memory operand

                            if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                                if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                                    currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                        }
                    }
                }
            } else{
                // other instructions can be deleted if possible
                
                if(op == "ADD" || op == "SUB" || op == "IMUL" || op == "AND" || op == "OR" || op == "XOR"){
                    // op1, op2 is read
                    // op1 is write

                    // if write (op1) is NOT in currentLive, this instruction is dead (need rsp/rbp check also)
                    if(currentLive.count(currBlock->getInstructions()[j].getOperand1()) == 0){
                        if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                            deleteVector.push_back(currBlock->getInstructions()[j].getId());
                            continue;
                        }
                    }

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand2());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                }

                else if(op == "SHL" || op == "SAR"){
                    // op1, op2(either rcx(cl) or constant) is getting read
                    // op1 is getting written
                    
                    // if write (op1) is NOT in currentLive, this instruction is dead (need rsp/rbp check also)
                    if(currentLive.count(currBlock->getInstructions()[j].getOperand1()) == 0){
                        if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                            deleteVector.push_back(currBlock->getInstructions()[j].getId());
                            continue;
                        }
                    }

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))                        
                            currentLive.insert("rcx");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                } 

                else if(op == "MOVZX"){
                    // op1, op2(either rax(al) or constant) is getting read
                    // op1 is getting written

                    // if write (op1) is NOT in currentLive, this instruction is dead (need rsp/rbp check also)
                    if(currentLive.count(currBlock->getInstructions()[j].getOperand1()) == 0){
                        if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                            deleteVector.push_back(currBlock->getInstructions()[j].getId());
                            continue;
                        }
                    }

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))                        
                            currentLive.insert("rax");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                } 

                else if(op == "NEG" || op == "NOT"){
                    // op1 is getting read
                    // op1 is getting overwritten

                    // if write (op1) is NOT in currentLive, this instruction is dead (need rsp/rbp check also)
                    if(currentLive.count(currBlock->getInstructions()[j].getOperand1()) == 0){
                        if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                            deleteVector.push_back(currBlock->getInstructions()[j].getId());
                            continue;
                        }
                    }

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                }
                
                else if(op == "IDIV"){
                    // rax,rdx are getting read
                    // op1 is getting read
                    // rax,rdx are getting overwritten

                    if(currentLive.count("rax") == 0 && currentLive.count("rdx") == 0){
                        if(!involves_stack_pointer(currBlock->getInstructions()[j].getOperand1(),currBlock->getInstructions()[j].getOperand2())){
                            deleteVector.push_back(currBlock->getInstructions()[j].getId());
                            continue;
                        }
                    }

                    currentLive.erase("rax");
                    currentLive.erase("rdx");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                    currentLive.insert("rax");
                    currentLive.insert("rdx");
                
                }

                else if(op == "CQO"){
                    // rax is getting read
                    // rdx is getting overwritten

                    if(currentLive.count("rdx") == 0){
                        deleteVector.push_back(currBlock->getInstructions()[j].getId());
                        continue;
                    }

                    currentLive.erase("rdx");

                    currentLive.insert("rax");
                } 

                else if(op == "JMP" || op == "JE" || op == "JNE" || op == "JL" || op == "JLE" || op == "JG" || op == "JGE" || isLabel(op)) continue;

                else if(op == "SETE" || op == "SETNE" || op == "SETL" || op == "SETLE" || op == "SETG" || op == "SETGE"){
                    // op1 (rax(al)) is getting overwritten
                    // RFLAGS are getting read , but we dont consider them here in this analysis

                    if(currentLive.count("rax") == 0){
                        deleteVector.push_back(currBlock->getInstructions()[j].getId());
                        continue;
                    }

                    currentLive.erase("rax");
                }
                else{
                    continue;
                }
                
            }
        }
    }

    // if(deleteVector.empty()) return;
    if(deleteVector.empty()) goto proceedToGraphColor; // using goto to skip under instrucitons and moving straight to graph color algo

    // removing the dead instructions and only keeping alive ones
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        auto currBlock = basic_blocks[i];

        optInstructions.clear();

        for(size_t j=0 ; j<currBlock->getInstructionsMutable().size() ; j++){
            if(find(deleteVector.begin() , deleteVector.end() , currBlock->getInstructionsMutable()[j].getId()) != deleteVector.end())
                continue;

            optInstructions.push_back(currBlock->getInstructionsMutable()[j]);
        }
        
        currBlock->getInstructionsMutable() = optInstructions;
                
    }

    // reseting vectors for the iterative approach
    deleteVector.clear();
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        basic_blocks[i]->use_set.clear();
        basic_blocks[i]->def_set.clear();
        basic_blocks[i]->live_in.clear();
        basic_blocks[i]->live_out.clear();
    }

    goto evaluateAgain;

    proceedToGraphColor:

    // now, we have the optimized 2-addr code instructions, we can start wiht out graph-color algo

    // interference graph        
    vector<Node> myGraph;

    // acting as helper to map string to id
    unordered_map<string , int> stringToIdMap;

    vector<string> physicalRegisterVector = { "rax", "rbx" , "rcx", "rdx","rsi","rdi","r8","r9","r10","r11","r12","r13","r14","r15"};
    
    // initializing pre-colored nodes
    for(int i=0 ; i<14 ; i++){
        Node temp;
        
        temp.isPhysical = true;
        temp.color = i;
        temp.degree = (int) INTMAX_MAX;
        temp.inGraph = true;
        temp.nodeId = i;
        temp.name = physicalRegisterVector[i];
        temp.spillCost = (int) INTMAX_MAX;

        stringToIdMap[temp.name] = temp.nodeId; 

        myGraph.push_back(temp);
        
    }

    // now the precolored nodes are place perfectly, we can proceed with making the edges of the graph

    // go through all the blocks
    for(size_t i=0 ; i<basic_blocks.size(); i++){
        auto currBlock = basic_blocks[i];

        // copy liveOut of currBlock to currentLive
        unordered_set<string> currentLive = currBlock->live_out;

        // make nodes for every string in currentLive
        for(const string& initString : currentLive){
            if(!isConstant(initString))
                if(!is_stack_pointer(initString)){
                    // if node of this string alr exist, continue
                    if(stringToIdMap.count(initString) != 0) continue;
                    // make new node and puhs to myGraph
                    Node temp;
                    temp.color = -1;
                    temp.degree = 0;
                    temp.inGraph = true;
                    temp.isPhysical = false;
                    temp.name = initString;
                    temp.nodeId = myGraph.size();
                    temp.spillCost = -1;
                    
                    stringToIdMap[initString] = myGraph.size();
                    myGraph.push_back(temp);
                }
        }

        // go through all instruction of currBlock in reverse order
        for(int j=currBlock->getInstructions().size()-1 ; j>=0 ; j--){
            string op = currBlock->getInstructions()[j].getOpcode();

            if(op == "MOV" || op == "LEA" || op == "PUSH" || op == "POP" || op == "CMP" || op == "CALL" || op == "ret"){
                // never delete instructions
                // MOV is never delete if dest is memory operand

                // ading lea here coz forgot to implement lea, since it has same logic as mov, i added it here with mov
                

                // it reads rax
                if(op == "ret") {                    
                    currentLive.insert("rax");
                } 
                // it reads op1 and op2 and updates RFLAGS(not counted as write btw)
                else if(op == "CMP"){
                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                                currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                } 

                // it reads op1 and pushes it's value to stack (rsp/rbp are NOT coutned as write in this phase)
                else if(op == "PUSH") {
                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                // it writes to op1 and updates the rsp/rbp
                } else if(op == "POP") {

                    drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);

                    // if op1 is present in currentLive, remove it
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());
                }
                // write to rax, and reads from registers containing the param values
                else if(op == "CALL"){

                    // caller-saved register edges
                    drawInterferenceEdges("rax" , currentLive , myGraph , stringToIdMap); // ret-value && caller saved both
                    drawInterferenceEdges("rcx" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("rdx" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("rsi" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("rdi" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("r8" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("r9" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("r10" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("r11" , currentLive , myGraph , stringToIdMap);


                    // remove caller-saved registers from currentLive
                    currentLive.erase("rax");
                    currentLive.erase("rcx");
                    currentLive.erase("rdx");
                    currentLive.erase("rsi");
                    currentLive.erase("rdi");
                    currentLive.erase("r8");
                    currentLive.erase("r9");
                    currentLive.erase("r10");
                    currentLive.erase("r11");      

                    // remove rax from currentLive 
                    currentLive.erase("rax");
                    
                    vector<string> paramReg;
                    paramReg.push_back("rdi");
                    paramReg.push_back("rsi");
                    paramReg.push_back("rdx");
                    paramReg.push_back("rcx");
                    paramReg.push_back("r8");
                    paramReg.push_back("r9");

                    // insert param registers to currentLive depending on the number of params
                    for(unsigned short k=0 ; k<stoi(currBlock->getInstructions()[j].getOperand2()) && k<6 ; k++){
                        currentLive.insert(paramReg[k]);
                    }

                    
                } else if(op == "MOV" || op == "LEA"){
                    if(is_memory_operand(currBlock->getInstructions()[j].getOperand1())){
                        // write part is memory operand, hence this instruction can never be deleted

                        if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){
                            // write and read, both are memory operand

                            // extract vars from write memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }

                            // extract vars from read memory part and insert in currentLive
                            holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }
                            
                        } else{
                            // only write is memory operand, read is register or var or const

                            // extract vars from write memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }

                            // insert read part to currentLive
                            if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                                if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))                                
                                    currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                        }
                    } else{
                        // wrtie part is NOT memory operand, hence it can be deleted if possible

                        drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);
                        
                        // now the write(op1) is in the currentLive

                        // remove op1 from currentLive
                        currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                        if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){
                            // write is NOT memory operand
                            // read is memory operand

                            // extract vars from read memory part and insert in currentLive
                            unordered_set<string> holder = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());
                            for(const string& tempStr : holder){
                                if(!isConstant(tempStr))
                                    if(!is_stack_pointer(tempStr))
                                        currentLive.insert(tempStr);
                            }
                        } else{
                            // neither of write or read is memory operand

                            if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                                if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                                    currentLive.insert(currBlock->getInstructions()[j].getOperand2());
                        }
                    }
                }
            } else{
                // other instructions can be deleted if possible
                
                if(op == "ADD" || op == "SUB" || op == "IMUL" || op == "AND" || op == "OR" || op == "XOR"){
                    // op1, op2 is read
                    // op1 is write

                    drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand2()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand2());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                }

                else if(op == "SHL" || op == "SAR"){
                    // op1, op2(either rcx(cl) or constant) is getting read
                    // op1 is getting written
                    
                    drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))                        
                            currentLive.insert("rcx");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                } 

                else if(op == "MOVZX"){
                    // op1, op2(either rax(al) or constant) is getting read
                    // op1 is getting written

                    drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand2()))                        
                            currentLive.insert("rax");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                } 

                else if(op == "NEG" || op == "NOT"){
                    // op1 is getting read
                    // op1 is getting overwritten

                    drawInterferenceEdges(currBlock->getInstructions()[j].getOperand1() , currentLive , myGraph , stringToIdMap);

                    // remove write from currentLive
                    currentLive.erase(currBlock->getInstructions()[j].getOperand1());

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());
                }
                
                else if(op == "IDIV"){
                    // rax,rdx are getting read
                    // op1 is getting read
                    // rax,rdx are getting overwritten

                    drawInterferenceEdges("rax" , currentLive , myGraph , stringToIdMap);
                    drawInterferenceEdges("rdx" , currentLive , myGraph , stringToIdMap);

                    currentLive.erase("rax");
                    currentLive.erase("rdx");

                    if(!isConstant(currBlock->getInstructions()[j].getOperand1()))
                        if(!is_stack_pointer(currBlock->getInstructions()[j].getOperand1()))
                            currentLive.insert(currBlock->getInstructions()[j].getOperand1());

                    currentLive.insert("rax");
                    currentLive.insert("rdx");
                
                }

                else if(op == "CQO"){
                    // rax is getting read
                    // rdx is getting overwritten

                    drawInterferenceEdges("rdx" , currentLive , myGraph , stringToIdMap);

                    currentLive.erase("rdx");

                    currentLive.insert("rax");
                } 

                else if(op == "JMP" || op == "JE" || op == "JNE" || op == "JL" || op == "JLE" || op == "JG" || op == "JGE" || isLabel(op)) continue;

                else if(op == "SETE" || op == "SETNE" || op == "SETL" || op == "SETLE" || op == "SETG" || op == "SETGE"){
                    // op1 (rax(al)) is getting overwritten
                    // RFLAGS are getting read , but we dont consider them here in this analysis

                    drawInterferenceEdges("rax" , currentLive , myGraph , stringToIdMap);

                    currentLive.erase("rax");
                }
                else{
                    continue;
                }
                
            }
        }
    }    

    // interference graph is ready now

    // printing graph
    printInterferenceGraph(myGraph);

    // now, we can proceed further

    vector<int> helperStack;
    bool sentLess14;

    sendLess14Label:

    sentLess14 = false;
    for(size_t i=0 ; i<myGraph.size() ; i++){

        // do NOT evaluate physical registers
        if(myGraph[i].isPhysical == true) continue;

        // evaluate only those which are currently in graph
        if(myGraph[i].inGraph == false) continue;

        // if degree is less than 14, push to helperStack and decr degree of all it's neighbours
        if(myGraph[i].degree < 14){
            
            // decr degree of all the neighbours
            for(const int& nId : myGraph[i].neigh){
                myGraph[nId].degree--;
            }

            // mark this node as NOT in graph
            myGraph[i].inGraph = false;
            
            // push nodeId to the stack
            helperStack.push_back(i);

            // mark that node wiht less than 14 edges is found
            sentLess14 = true;
        }
    }

    // if any node less than 14 degree was found, go through the loop again
    if(sentLess14) goto sendLess14Label;

    // now all the other var have degree more than 14, now, we need to decide which var to push to stack first,
    // in this project, we will push the highest-degree var to stack first to keep it simple

    long long highestDegreeIndex = -1;

    for(size_t i=0 ; i<myGraph.size() ; i++){

        // do NOT evaluate physical registers
        if(myGraph[i].isPhysical == true) continue;

        // evaluate only those which are currently in graph
        if(myGraph[i].inGraph == false) continue;

        // degree is 14 or more
        if(myGraph[i].degree >= 14){
            if(highestDegreeIndex == -1){
                highestDegreeIndex = i;
            } else{
                if(myGraph[i].degree > myGraph[highestDegreeIndex].degree){
                    highestDegreeIndex = i;
                }
            }
        }
        
    }

    if(highestDegreeIndex != -1) {

        // decr degree of all the neighbours
        for(const int& nId : myGraph[highestDegreeIndex].neigh){
            myGraph[nId].degree--;
        }

        // set the node that is pushed to be NOT in graph anymore
        myGraph[highestDegreeIndex].inGraph = false;
        
        // push the nodeId to the graph
        helperStack.push_back(highestDegreeIndex);

        // reset the helperIndex
        highestDegreeIndex = -1;

        // check again
        goto sendLess14Label;
    }

    // here, all nodes have been succesfully pushed to the stack

    printPhase4Stack(myGraph, helperStack);

    // we can now start popping nodes from the stack and proceed accordingly

    // sequence of assinging registers (calleer-saved , then calle saved)
    // rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11, rbx, r12, r13, r14, r15
    vector<int> priorityRegAllocationSequence = {0 , 2 , 3 , 4, 5, 6, 7, 8, 9 , 1 , 10, 11, 12, 13};
    
    unordered_map<string , int> varRegMap;
    vector<int> spillNeeded;

    // keep popping till the stack is empty
    while(helperStack.size() != 0){
        
        // store teh last value in the stack
        int popedVar = helperStack[helperStack.size()-1];

        // mark the reg as NA for now
        int regAvail = -1;

        // copy the possible registers 
        vector<int> checkRegAvail = priorityRegAllocationSequence;

        // go through all neigh of the popedNode
        for(const int& currNeigh : myGraph[popedVar].neigh){

            // if node not in graph currently, continue
            if(!myGraph[currNeigh].inGraph) continue;

            // if neigh is physical register, continue
            // if(myGraph[currNeigh].isPhysical) continue;

            // remove the possibilities of assinging the neigh color to our poped node
            checkRegAvail.erase(remove(checkRegAvail.begin(), checkRegAvail.end(), myGraph[currNeigh].color),checkRegAvail.end());          
        }

        // if any color index remians in the set, assign the first color index
        if(!checkRegAvail.empty()) regAvail = *checkRegAvail.begin();

        if(regAvail != -1){
            // reg is found, add in the mapping
            varRegMap[myGraph[popedVar].name] = regAvail;
            myGraph[popedVar].inGraph = true;
            myGraph[popedVar].color = regAvail;

        } else{
            // reg not found, mark to be spilled
            spillNeeded.push_back(popedVar);
        }

        // pop the element from the stack
        helperStack.pop_back();
        
    }

    

    /*
        we are gonna now make a hashmap of which var belong to which func
        this will be used for determinig what offset to se for each spileld var
        this will be used in determining and solving the problem of callee saved registers
    */

    string currFunc = "";

    // go throguh all the blocks linearly
    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        // go throguh all instructions linearly frm starting of each block
        auto currBlock = basic_blocks[i];
        for(size_t j=0 ; j<currBlock->getInstructions().size() ; j++){

            // this is new func starting label
            if(currBlock->getInstructions()[j].getOpcode() == "FUNC"){
                // update currFunc 
                currFunc = currBlock->getInstructions()[j].getOperand1();

                // if mapping of this func to it's offset isnt alr present, initialize it
                if(funcOffsetMap.count(currFunc) == 0) funcOffsetMap[currFunc] = 0;
            }

            unordered_set<string> helperOpStorage;
            if(!isConstant(currBlock->getInstructions()[j].getOperand1()) && 
                currBlock->getInstructions()[j].getOperand1() != "" &&
                !isPhysicalRegister(currBlock->getInstructions()[j].getOperand1()) &&
                !is_memory_operand(currBlock->getInstructions()[j].getOperand1())){                
                    
                    // add if not constant && not memory operand && snot physical register 
                    helperOpStorage.insert(currBlock->getInstructions()[j].getOperand1());
            }
            if(!isConstant(currBlock->getInstructions()[j].getOperand2()) && 
                currBlock->getInstructions()[j].getOperand2() != "" &&
                !isPhysicalRegister(currBlock->getInstructions()[j].getOperand2()) &&
                !is_memory_operand(currBlock->getInstructions()[j].getOperand2())){                
                    
                    // add if not constant && not memory operand && snot physical register
                    helperOpStorage.insert(currBlock->getInstructions()[j].getOperand2());
            }

            // case for op1 being memory operand
            if(is_memory_operand(currBlock->getInstructions()[j].getOperand1())){

                // extracting vars
                unordered_set<string> tempStorage = extract_memory_vars(currBlock->getInstructions()[j].getOperand1());
                for(const string& each : tempStorage){

                    // add if not constant && not physical register
                    if(!isConstant(each) && !isPhysicalRegister(each))   
                        helperOpStorage.insert(each);
                }
            }
            if(is_memory_operand(currBlock->getInstructions()[j].getOperand2())){

                // extracting vars
                unordered_set<string> tempStorage = extract_memory_vars(currBlock->getInstructions()[j].getOperand2());
                for(const string& each : tempStorage){

                    // add if not constant && not physical register
                    if(!isConstant(each) && !isPhysicalRegister(each))                                        
                        helperOpStorage.insert(each);
                }
            }

            // check if list is empty
            if(!helperOpStorage.empty()){
                for(const string& eachInHelper : helperOpStorage){

                    // insert the mapping if not present
                    if(varFuncMap.count(eachInHelper) == 0){ 
                        // create the mapping
                        varFuncMap[eachInHelper] = currFunc;
                    }
                }
            }

            
        }
    }

    printPhase4MappingAndSpills(varRegMap, spillNeeded, myGraph, varFuncMap);


    // now, we need to deal with the variables in the spillNeeded vector

    
    unordered_map<string , string> replacerMap;

    bool isSpillNeededEmpty = spillNeeded.empty(); 

    for(size_t i=0 ; i<spillNeeded.size() ; i++){

        // store the index of the current node that is being dealt with
        int currNode = spillNeeded[i];

        // name of currentPoped var
        string spillName = myGraph[currNode].name;

        string stackAddr;

        string funcAttachToThisVar = varFuncMap[spillName];
        int countOfOffsetUsed = funcOffsetMap[funcAttachToThisVar] + 1;

        stackAddr = "[rbp - " + to_string(8*countOfOffsetUsed) + "]"; 
        funcOffsetMap[funcAttachToThisVar] = countOfOffsetUsed;

        replacerMap[spillName] = stackAddr;

        int helperCount = 1;

        // clearing the vector for a fresh start
        modifiedInstructions.clear();

        // go through all blocks
        for(size_t j=0 ; j<basic_blocks.size() ; j++){

            // go through all insturctions of this block
            for(size_t k=0 ; k<basic_blocks[j]->getInstructions().size() ; k++){

                // if spillName is NOT in this instruciton, copy that to modified instructions as it is
                if(basic_blocks[j]->getInstructions()[k].getOperand1() != spillName && basic_blocks[j]->getInstructions()[k].getOperand2() != spillName){
                    modifiedInstructions.push_back(basic_blocks[j]->getInstructions()[k]);
                    continue;
                }

                // copy the current instruction
                auto currInstruction = basic_blocks[j]->getInstructions()[k];
                string originalOp1 = currInstruction.getOperand1();
                string originalOp2 = currInstruction.getOperand2();           
                
                string specialCase = "";

                if(originalOp1 == spillName && isOp1Read(currInstruction.getOpcode())){
                    // op1 is spillName and getting read

                    string helperName = spillName + "_helper_" + to_string(helperCount++);
                    specialCase = helperName;

                    modifiedInstructions.push_back(makeInstruction("MOV" , helperName , stackAddr));
                    
                    currInstruction.setOperand1(helperName);
                } 

                if(originalOp2 == spillName && isOp2Read(currInstruction.getOpcode())){
                    // op2 is spillName and getting read                    

                    string helperName = spillName + "_helper_" + to_string(helperCount++);
                    modifiedInstructions.push_back(makeInstruction("MOV" , helperName , stackAddr));
                    currInstruction.setOperand2(helperName);
                } 

                
                if(originalOp1 == spillName && isOp1Modified(currInstruction.getOpcode())){
                    // op1 is spillName and getting modified
                    
                    string helperName = spillName + "_helper_" + to_string(helperCount++);
                    
                    if(specialCase == "")
                        currInstruction.setOperand1(helperName);                    
                    else
                        currInstruction.setOperand1(specialCase);                    
                    
                    modifiedInstructions.push_back(currInstruction);
                    
                    if(specialCase == "")
                        modifiedInstructions.push_back(makeInstruction("MOV" , stackAddr , helperName));
                    else
                        modifiedInstructions.push_back(makeInstruction("MOV" , stackAddr , specialCase));
                } else{
                    modifiedInstructions.push_back(currInstruction);
                }
            }

            basic_blocks[j]->getInstructionsMutable() = modifiedInstructions;
            
            modifiedInstructions.clear();
        }
    }

    // now the spillStack is empty and if it was NOT empty beofre, we need to start the entire live analysis again

    if(!isSpillNeededEmpty){
        // run the entire live analysis again

        for(size_t i=0 ; i<basic_blocks.size() ; i++){
            basic_blocks[i]->use_set.clear();
            basic_blocks[i]->def_set.clear();
            basic_blocks[i]->live_in.clear();
            basic_blocks[i]->live_out.clear();
        }

        goto runLiveAnalysisAgain;
    }

    // now, all the variables have been properly spilled and proeprly mapped to registers
    // varRegMap contains the proper mapping

    /*
        now, we need to take care of the callee-saved registers, if they are used, then we need to save them to stack and bring them back when there is ret
        we are gonna use smae offset map since their locaiton will be allocated with the spillSection itself
    */

    currFunc = "";

    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        for(size_t j=0 ; j<basic_blocks[i]->getInstructions().size() ; j++){

            if(basic_blocks[i]->getInstructions()[j].getOpcode() == "FUNC"){
                currFunc = basic_blocks[i]->getInstructions()[j].getOperand1();
                continue;
            }
            
            auto currInstruction = basic_blocks[i]->getInstructions()[j];

            string op1 = currInstruction.getOperand1();
            string op2 = currInstruction.getOperand2();

            if(!isOp1Modified(currInstruction.getOpcode())) continue;

            // if(replacerMap.count(op1) != 0) continue;

            unordered_set<string> calleeSaved = {"rbx" , "r12" , "r13" , "r14" , "r15"};
            unordered_set<int> calleeSavedId = {1 , 10 , 11 , 12 , 13};

            unordered_set<string> helperSet;

            if(calleeSaved.count(op1) != 0){                
                string phyReg = op1;

                if(calleeSavedMap[currFunc].count(phyReg) != 0) continue;
                
                if(funcCalleeOffsetMap.count(currFunc) == 0){
                    funcCalleeOffsetMap[currFunc] = 0;
                }

                int offset = funcCalleeOffsetMap[currFunc]++ + funcOffsetMap[currFunc] + 1;

                string stackAddr = "[rbp - " + to_string(offset * 8) + "]";

                // replacerMap[phyReg] = stackAddr;
                calleeSavedMap[currFunc][phyReg] = stackAddr; 

            } else if(calleeSavedId.count(varRegMap[op1]) != 0){
                string phyReg = getPhysicalRegisterName(varRegMap[op1]);
                
                string func = varFuncMap[op1];

                if(calleeSavedMap[func].count(phyReg) != 0) continue;

                if(funcCalleeOffsetMap.count(func) == 0){
                    funcCalleeOffsetMap[func] = 0;
                }

                int offset = funcCalleeOffsetMap[func]++ + funcOffsetMap[func] + 1;

                string stackAddr = "[rbp - " + to_string(offset * 8) + "]";

                // replacerMap[op1] = stackAddr;
                calleeSavedMap[func][phyReg] = stackAddr; 
            }

        }
    }

    // now, we everything we need for the final asm generation
    // spilled vars addresses are stored in replacerMap and the callee-saved registers addresses are stored n calleeSavedMap

    modifiedInstructions.clear();

    currFunc = "";

    for(size_t i=0 ; i<basic_blocks.size() ; i++){
        auto currBlock = basic_blocks[i];
        for(size_t j=0 ; j<currBlock->getInstructions().size() ; j++){
            auto currInstruction = currBlock->getInstructions()[j];

            if(currInstruction.getOpcode() == "FUNC"){
                // prolouge

                currFunc = currInstruction.getOperand1();

                // get the total number of vars
                // vars spilled for this func and the callee-saved vars being accessed by this func
                int count = funcOffsetMap[currInstruction.getOperand1()] + funcCalleeOffsetMap[currInstruction.getOperand1()];

                // if count is even, it is perfectly 16-byte aligned, if odd, add extra 8-bytes to align it
                string offset = to_string(count%2 == 0 ? count*8 : count*8 + 8);

                // push the funcInstruction
                modifiedInstructions.push_back(currInstruction);

                // setting the rsp and rbp for the func
                modifiedInstructions.push_back(makeInstruction("PUSH" , "rbp"));
                modifiedInstructions.push_back(makeInstruction("MOV" , "rbp" , "rsp"));

                // prolouge instruction
                modifiedInstructions.push_back(makeInstruction("SUB" , "rsp" , offset));

                // now, add MOV intructions for callee saved registers
                // the callee-saved vars being accessed by this func
                count = funcCalleeOffsetMap[currInstruction.getOperand1()];

                for(const auto& func : calleeSavedMap[currFunc]){
                    string phy = func.first;
                    string stackAddr = func.second;
                    
                    modifiedInstructions.push_back(makeInstruction("MOV" , stackAddr , phy));
                }

                continue;
            }
            
            if(j+2 < currBlock->getInstructions().size() && currBlock->getInstructions()[j+2].getOpcode() == "ret"){
                // epilouge
                // current instruction is      MOV rbp , rsp
                // next instruction is         POP rbp
                // next to next instruction is ret

                
                // the callee-saved vars being accessed by this func
                // int count = funcCalleeOffsetMap[currInstruction.getOperand1()];

                for(const auto& func : calleeSavedMap[currFunc]){
                    string phy = func.first;
                    string stackAddr = func.second;
                    
                    modifiedInstructions.push_back(makeInstruction("MOV" , phy , stackAddr));
                }

                modifiedInstructions.push_back(currBlock->getInstructions()[j]); // adding MOV rbp , rsp
                modifiedInstructions.push_back(currBlock->getInstructions()[j+1]); // adding POP rbp
                modifiedInstructions.push_back(currBlock->getInstructions()[j+2]); // adding ret

                j += 2;

                continue;
            } 
            
            // other instructions, replace the var wiht their register mappings and push 
            string op1 = currInstruction.getOperand1();
            string op2 = currInstruction.getOperand2();
            
            if(!isConstant(op1) && !isPhysicalRegister(op1) && op1 != "" && !isJumpOrCall(currInstruction.getOpcode())){
                if(is_memory_operand(op1)){                    
                    currInstruction.setOperand1(map_memory_operand(op1, varRegMap));
                } else{
                    currInstruction.setOperand1(getPhysicalRegisterName(varRegMap[op1]));
                }
            }

            if(!isConstant(op2) && !isPhysicalRegister(op2) && op2 != "" && !isJumpOrCall(currInstruction.getOpcode())){
                if(is_memory_operand(op2)){
                    currInstruction.setOperand2(map_memory_operand(op2, varRegMap));
                } else{
                    currInstruction.setOperand2(getPhysicalRegisterName(varRegMap[op2]));
                }
            }

            modifiedInstructions.push_back(currInstruction);
                            
        }

        currBlock->getInstructionsMutable() = modifiedInstructions;
        modifiedInstructions.clear(); // Clear it for the next basic block!
    }


}

std::vector<TwoAddressInstruction> ControlFlowGraph::getOptimizedInstructions() const {
    std::vector<TwoAddressInstruction> flat_list;
    for (const auto& block : basic_blocks) {
        for (const auto& instr : block->getInstructions()) {
            flat_list.push_back(instr);
        }
    }
    return flat_list;
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



// Helper: Convert graph color ID to physical x86-64 register string
inline std::string getPhysicalRegisterName(int id) {
    static const std::string regNames[] = {
        "rax", "rbx", "rcx", "rdx", "rsi", "rdi", 
        "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15"
    };
    
    if (id >= 0 && id < 14) {
        return regNames[id];
    }
    
    return "UNKNOWN_REG"; // Fallback, should never be hit if coloring was successful
}





