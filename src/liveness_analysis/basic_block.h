#ifndef RM_FORGE_BASIC_BLOCK_H
#define RM_FORGE_BASIC_BLOCK_H

#include <vector>
#include <unordered_set>
#include <string>
#include <memory>
#include "../tac_to_2ac/two_address_instruction.h"

namespace rm_forge {

class BasicBlock {    
public:
    BasicBlock(int id);
    
    // Add a 2AC instruction to this block
    void addInstruction(const TwoAddressInstruction& instr);

    void print() const;

    // CFG Edge Management
    void addSuccessor(std::shared_ptr<BasicBlock> succ);
    void addPredecessor(std::shared_ptr<BasicBlock> pred);

    int getId() const { return block_id; }
    const std::vector<TwoAddressInstruction>& getInstructions() const { return instructions; }
    std::vector<TwoAddressInstruction>& getInstructionsMutable() { return instructions; }

    // ---------------------------------------------------------
    // Liveness Analysis Sets (Using string for virtual registers)
    // ---------------------------------------------------------
    std::unordered_set<std::string> use_set;
    std::unordered_set<std::string> def_set;
    std::unordered_set<std::string> live_in;
    std::unordered_set<std::string> live_out;

    // ---------------------------------------------------------
    // Control Flow Graph Edges
    // ---------------------------------------------------------
    std::vector<std::shared_ptr<BasicBlock>> cfg_out; // Successors
    std::vector<std::shared_ptr<BasicBlock>> cfg_in;  // Predecessors

private:
    int block_id;
    std::vector<TwoAddressInstruction> instructions;
};

} // namespace rm_forge

#endif // RM_FORGE_BASIC_BLOCK_H
