#include "converter.h"

using namespace std;

namespace rm_forge {

TacTo2acConverter::TacTo2acConverter(const std::vector<Instruction>& tac_instructions)
    : tac_instructions_(tac_instructions) {}



const std::vector<TwoAddressInstruction>& TacTo2acConverter::get_2ac_instructions() const {
    return two_addr_instructions_;
}

void TacTo2acConverter::convert() {
    // this is the 3-addr code vector
    // tac_instructions_

    // this is the vector that will be the output
    // two_addr_instructions_

    for(size_t i=0 ; i<tac_instructions_.size() ; i++){
        Instruction current = tac_instructions_[i];
        if(current.get_type() == InstructionType::Assignment){
            handleAssignment(&current);
        }
        else if(current.get_type() == InstructionType::BinaryOperation){
            handleBinary(&current);
        } else if(current.get_type() == InstructionType::UnaryOperation){
            handleUnary(&current);
        } else if(current.get_type() == InstructionType::Return){
            handleReturn(&current);
        } else if(current.get_type() == InstructionType::Label){
            handleLabel(&current);
        } else if(current.get_type() == InstructionType::UnconditionalGoto){
            handleUnconditionalGoto(&current);
        } else if(current.get_type() == InstructionType::ConditionalGoto){
            handleConditionalGoto(&current);
        } else if(current.get_type() == InstructionType::Parameter){
            unsigned short skip = handleParam(tac_instructions_,i);
            i += skip-1;
        } else if(current.get_type() == InstructionType::FunctionCall || current.get_type() == InstructionType::FunctionCallWithReturn){
            handleCall(&current);
        } else if(current.get_type() == InstructionType::ArrayLoad || current.get_type() == InstructionType::ArrayStore){
            handleArray(&current);
        } else if(current.get_type() == InstructionType::PointerLoad || current.get_type() == InstructionType::PointerStore){
            handlePointor(&current);
        } else if(current.get_type() == InstructionType::AddressOf){
            handleAddressOf(&current);
        }
        else{
            continue;
        }
    }

    return;
}

string* TacTo2acConverter::getNewTempRegister(){
    string* tempName = new string();
    *tempName = "s" + to_string(tempRegisterCount);
    tempRegisterCount++;

    return tempName;    
}

void TacTo2acConverter::handleAssignment(Instruction* current){
    // string* reg = getNewTempRegister();

    // emit("MOV" , *reg , current->get_result());
    
    emit("MOV" , current->get_result() , current->get_arg1());

    return;
}

void TacTo2acConverter::handleBinary(Instruction* current){
    string op = current->get_op();    

    if(op == "+"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());                             
        emit("ADD" , current->get_result() , current->get_arg2());
        
    } else if(op == "-"){
        if(current->get_result() != current->get_arg1())          
            emit("MOV" , current->get_result() , current->get_arg1());        
        emit("SUB" , current->get_result() , current->get_arg2());
        
    } else if(op == "*"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());        
        emit("IMUL" , current->get_result() , current->get_arg2());        
    } else if(op == "&" || op == "&&"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("AND" , current->get_result() , current->get_arg2());
    } else if(op == "|" || op == "||"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("OR" , current->get_result() , current->get_arg2());
    } else if(op == "^"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("XOR" , current->get_result() , current->get_arg2());
    } else if(op == "<<"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("MOV" , "rcx" , current->get_arg2());
        emit("SHL" , current->get_result() , "cl");                
    } else if(op == ">>"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("MOV" , "rcx" , current->get_arg2());
        emit("SAR" , current->get_result() , "cl");    
    } else if(op == "/"){
        emit("MOV" , "rax" ,current->get_arg1());
        emit("CQO" , "" , "");
        emit("IDIV" , current->get_arg2() , "");

        emit("MOV" , current->get_result() , "rax");
    } else if(op == "%"){
        emit("MOV" , "rax" , current->get_arg1());
        emit("CQO" , "" , "");
        emit("IDIV" , current->get_arg2() , "");

        emit("MOV" , current->get_result() , "rdx");
    } else if(op == "==" || op == "!=" || op == "<" || op == "<=" || op == ">" || op == ">="){
        emit("CMP" , current->get_arg1() , current->get_arg2());

        if(op == "==") emit("SETE" , "al" , "");
        else if(op == "!=") emit("SETNE" , "al" , "");
        else if(op == "<") emit("SETL" , "al" , "");
        else if(op == "<=") emit("SETLE" , "al" , "");
        else if(op == ">") emit("SETG" , "al" , "");
        else if(op == ">=") emit("SETGE" , "al" , "");

        emit("MOVZX" , current->get_result() , "al");
    }

    return;
}

void TacTo2acConverter::handleUnary(Instruction* current){
    if(current->get_op() == "-"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("NEG" , current->get_result() , "");            
    } else if(current->get_op() == "~"){
        if(current->get_result() != current->get_arg1())
            emit("MOV" , current->get_result() , current->get_arg1());
        emit("NOT" , current->get_result() , "");
    } else if(current->get_op() == "!"){
        emit("CMP" , current->get_arg1() , "0");
        emit("SETE" , "al" , "");
        emit("MOVZX" , current->get_result() , "al");
    }

    return;
}

void TacTo2acConverter::handleReturn(Instruction* current){

    // need to reset rsp and rbp
    // bring rsp to rbp position
    emit("MOV" , "rsp" , "rbp");

    // using pop, since pop is equivalent to below 2 commands
    emit("POP" , "rbp" , "");

    // bring rbp back to it's parent func rbp
    // emit("MOV" , "rbp" , "[rsp]");

    // move rsp 1 step up
    // emit("ADD" , "rsp" , "8");   

    if(current->get_arg1() == ""){
        // return void
        emit("ret" , "" , "");
    } else{        
        // setting return value to rax
        emit("MOV" , "rax" , current->get_arg1());
        emit("ret" , "" , "");
    }

    return;
}

void TacTo2acConverter::handleLabel(Instruction* current){
    emit(current->get_result(),"","");
    return;
}

void TacTo2acConverter::handleUnconditionalGoto(Instruction* current){

    emit("JMP" , current->get_result(),"");

    return;
}

void TacTo2acConverter::handleConditionalGoto(Instruction* current){
    emit("CMP" , current->get_arg1() , "0");

    if(current->get_op() == "if"){
        emit("JNE" , current->get_result());
    } else{
        emit("JE" , current->get_result());
    }

    return;
}

unsigned short TacTo2acConverter::handleParam(vector<Instruction> list , int index){

    vector<string> helper;
    helper.push_back("rdi");
    helper.push_back("rsi");
    helper.push_back("rdx");
    helper.push_back("rcx");
    helper.push_back("r8");
    helper.push_back("r9");

    unsigned short paramCount = 0; 

    evaluateParam:
    
    if(list[index].get_type() == InstructionType::Parameter){
        if(paramCount < 6){
            // emit("MOV" , *getNewTempRegister() , helper[paramCount]);
            emit("MOV" , helper[paramCount] , list[index].get_arg1());
            paramCount++;
            index++;
        } else{
            emit("PUSH" , list[index].get_arg1() , "");
            paramCount++;
            index++;
        }
        
    }else{
        return paramCount;
    }

    goto evaluateParam;
    
}

void TacTo2acConverter::handleCall(Instruction* current){
    if(current->get_type() == InstructionType::FunctionCall){
        emit("CALL" , current->get_arg1() , current->get_arg2());
    } else{
        emit("CALL" , current->get_arg1() , current->get_arg2());
        emit("MOV" , current->get_result() , "rax");
    }

    return;
}

void TacTo2acConverter::handleArray(Instruction* current){
    if(current->get_type() == InstructionType::ArrayLoad){ 
        // e.g., a = b[i]

        // emit("MOV" , *getNewTempRegister() , current->get_result());
        string addressFormula = "[" + current->get_arg1() + " + " + current->get_arg2() + "*8]";
        emit("MOV" , current->get_result() , addressFormula);
    } else{
        // e.g., a[i] = b

        string addressFormula = "[" + current->get_result() + " + " + current->get_arg1() + "*8]";
        // emit("MOV" , *getNewTempRegister() , addressFormula);
        emit("MOV" , addressFormula , current->get_arg2());
    }

    return;
}

void TacTo2acConverter::handlePointor(Instruction* current){
    if(current->get_type() == InstructionType::PointerLoad){
        // e.g., a = *b (Dereference right side)
        emit("MOV" , current->get_result() , "[" + current->get_arg1() + "]");
    } else{
        // e.g., *a = b (Dereference left side)
        emit("MOV" , "[" + current->get_result() + "]" , current->get_arg1());
    }

    return;
}

void TacTo2acConverter::handleAddressOf(Instruction* current){
    emit("LEA" , current->get_result() , "[" + current->get_arg1() + "]");

    return;
}


void TacTo2acConverter::emit(const string& opcode, const string& operand1, const string& operand2) {
    two_addr_instructions_.push_back(TwoAddressInstruction(opcode, operand1, operand2));
}

} // namespace rm_forge
