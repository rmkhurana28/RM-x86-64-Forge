#include "basic_block.h"
#include <iostream>

namespace rm_forge {

BasicBlock::BasicBlock(int id) : block_id(id) {
}

void BasicBlock::addInstruction(const TwoAddressInstruction& instr) {
    instructions.push_back(instr);
}

void BasicBlock::addSuccessor(std::shared_ptr<BasicBlock> succ) {
    cfg_out.push_back(succ);
}

void BasicBlock::addPredecessor(std::shared_ptr<BasicBlock> pred) {
    cfg_in.push_back(pred);
}

void BasicBlock::print() const {
    std::cout << "==========================================\n";
    std::cout << "Block ID: " << block_id << "\n";
    std::cout << "------------------------------------------\n";
    std::cout << "Instructions:\n";
    for (const auto& instr : instructions) {
        instr.print();
    }
    std::cout << "------------------------------------------\n";
    
    std::cout << "CFG IN:   { ";
    if (cfg_in.empty()) std::cout << "Φ ";
    else for (const auto& pred : cfg_in) std::cout << pred->getId() << " ";
    std::cout << "}\n";

    std::cout << "CFG OUT:  { ";
    if (cfg_out.empty()) std::cout << "Φ ";
    else for (const auto& succ : cfg_out) std::cout << succ->getId() << " ";
    std::cout << "}\n";
    
    std::cout << "------------------------------------------\n";

    std::cout << "DEF:      { ";
    if (def_set.empty()) std::cout << "Φ ";
    else for (const auto& v : def_set) std::cout << v << " ";
    std::cout << "}\n";

    std::cout << "USE:      { ";
    if (use_set.empty()) std::cout << "Φ ";
    else for (const auto& v : use_set) std::cout << v << " ";
    std::cout << "}\n";

    std::cout << "LIVE IN:  { ";
    if (live_in.empty()) std::cout << "Φ ";
    else for (const auto& v : live_in) std::cout << v << " ";
    std::cout << "}\n";

    std::cout << "LIVE OUT: { ";
    if (live_out.empty()) std::cout << "Φ ";
    else for (const auto& v : live_out) std::cout << v << " ";
    std::cout << "}\n";

    std::cout << "==========================================\n\n";
}

} // namespace rm_forge
