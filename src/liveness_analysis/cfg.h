#ifndef RM_FORGE_CFG_H
#define RM_FORGE_CFG_H

#include <vector>
#include <memory>
#include "basic_block.h"
#include "../tac_to_2ac/two_address_instruction.h"

namespace rm_forge {

inline bool is_stack_pointer(const std::string& op) {
    return op == "rsp" || op == "rbp";
}

inline bool involves_stack_pointer(const std::string& op1, const std::string& op2) {
    return op1 == "rsp" || op1 == "rbp" || op2 == "rsp" || op2 == "rbp";
}

class ControlFlowGraph {
public:
    ControlFlowGraph();

    // ---------------------------------------------------------
    // Phase 2 Entry Points
    // ---------------------------------------------------------
    
    // Builds the CFG from a flat list of 2AC instructions
    void buildCFG(const std::vector<TwoAddressInstruction>& instructions);

    // Computes USE, DEF, IN, and OUT sets for all basic blocks
    void computeLiveness();

    // Helper: Creates a new block, adds it to the graph, and returns it
    std::shared_ptr<BasicBlock> createNewBlock();

    // Prints the CFG structure
    void print() const;

    const std::vector<std::shared_ptr<BasicBlock>>& getBlocks() const { return basic_blocks; }

private:
    std::vector<std::shared_ptr<BasicBlock>> basic_blocks;
    int next_block_id = 0;

    // Helper functions for block identification
    bool isUnconditionalJump(const std::string& opcode) const;
    bool isConditionalJump(const std::string& opcode) const;
    bool isReturn(const std::string& opcode) const;
    bool isLastInstruction(size_t index, size_t total_size) const;
    bool isLabel(const std::string& opcode) const;
};

} // namespace rm_forge

#endif // RM_FORGE_CFG_H
