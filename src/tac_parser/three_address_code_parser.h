#ifndef RM_FORGE_THREE_ADDRESS_CODE_PARSER_H
#define RM_FORGE_THREE_ADDRESS_CODE_PARSER_H

#include "instruction.h"
#include <string>
#include <vector>

namespace rm_forge {

class ThreeAddressCodeParser {
public:
    explicit ThreeAddressCodeParser(const std::string& filepath);

    bool parse();
    const std::vector<Instruction>& get_instructions() const;

private:
    std::string filepath_;
    std::vector<Instruction> instructions_;
    
    Instruction parse_line(const std::string& line) const;
};

} // namespace rm_forge

#endif // RM_FORGE_THREE_ADDRESS_CODE_PARSER_H
