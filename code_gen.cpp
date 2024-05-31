#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include "Core.h"
#include <ostream>
#include <iostream>
#include <fstream>

using namespace std;

// global vars
unordered_map<LLVMValueRef, int> offset_map;

// helper functions
unordered_map<LLVMBasicBlockRef, string> createBBLabels(LLVMModuleRef module) {
    unordered_map<LLVMBasicBlockRef, string> bb_labels;
    int bb_count = 0;
    for (LLVMValueRef func = LLVMGetFirstFunction(module); func; func = LLVMGetNextFunction(func)) {
        for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb; bb = LLVMGetNextBasicBlock(bb)) {
            bb_labels[bb] = "BB" + to_string(bb_count);
            bb_count++;
        }
    }
    return bb_labels;
}

void printDirectives(ofstream& file) {
    file << ".text \n";
    file << ".globl func\n";
    file << ".type func\n";
}

void printFunctionEnd(ofstream& file){
    file << "pop %ebx\n";
    file << "leave\n";
    file << "ret\n";
}

// input: module and empty offset map
// output: localMemSize
int initOffsetMap(LLVMModuleRef module, unordered_map<LLVMValueRef, int> &offset_map){
    // print out offset map
    // cout << "printing offset map: \n";
    // for(auto it = offset_map.begin(); it != offset_map.end(); it++){
    //     cout << "key: " << LLVMPrintValueToString(it->first) << " value: " << it->second << "\n";
    // }
    // populate the offset map
    int localMem = 4;
    // check if function has parameter
    LLVMValueRef func = LLVMGetFirstFunction(module);
    if(LLVMCountParams(func) > 0){
        // there will only be 1 param
        LLVMValueRef param = LLVMGetParam(func, 0);
        offset_map[param] = 8;
    }
    // loop through each basic block
    for(LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb; bb = LLVMGetNextBasicBlock(bb)){
        // loop through each instruction
        for(LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)){
            if(LLVMGetInstructionOpcode(inst) == LLVMAlloca){
                offset_map[inst] = -1*localMem;
                localMem += 4;
            }
            else if (LLVMGetInstructionOpcode(inst) == LLVMStore){
                // check if the first opand of the store inst is equal to function param
                if(LLVMGetOperand(inst, 0) == LLVMGetParam(func, 0)){
                    int x = offset_map[LLVMGetOperand(inst, 0)];
                    offset_map[LLVMGetOperand(inst, 1)] = x;
                } else {
                    int x = offset_map[LLVMGetOperand(inst, 1)];
                    offset_map[LLVMGetOperand(inst, 0)] = x;
                }
            }
            else if (LLVMGetInstructionOpcode(inst) == LLVMLoad){
                int x = offset_map[LLVMGetOperand(inst, 0)];
                offset_map[inst] = x;
            }
        }
    }
    return localMem;
}

// helper function: check if a variable is memory or in register
bool isInMemory(LLVMValueRef value){
    return (offset_map.find(value) != offset_map.end());
}


bool isInRegister(LLVMValueRef value, unordered_map<LLVMValueRef, int> reg_map){
    return (reg_map.find(value) != reg_map.end() && reg_map[value] != -1);
}

bool isParam(LLVMValueRef value){
    // Iterate through all uses of the instruction
    for (LLVMUseRef use = LLVMGetFirstUse(value); use != NULL; use = LLVMGetNextUse(use)){
        LLVMValueRef user = LLVMGetUser(use);

        // Check if the user is a store instruction
        if (LLVMIsAStoreInst(user))
        {
            LLVMValueRef storedValue = LLVMGetOperand(user, 0);

            // Check if the stored value is an argument
            if (LLVMIsAArgument(storedValue))
            {
                return true;
            }
        }
    }
    return false;
}

// helper function: used to generate code for different predicate labels
static std::string
getAssemblyOpcodeForPredicate(LLVMIntPredicate predicate)
{
    switch (predicate)
    {
    case LLVMIntEQ:
        return "je";
    case LLVMIntNE:
        return "jne";
    case LLVMIntSGT:
        return "jg";
    case LLVMIntSGE:
        return "jge";
    case LLVMIntSLT:
        return "jl";
    case LLVMIntSLE:
        return "jle";
    default:
    {
        cout << "Unsupported comparison predicate\n";
        return nullptr;
    }
    }
}


// helper function: map register index to register name
string mapReg(int index){
    if(index == 1){
        return "%ebx";
    }
    else if(index == 2){
        return "%ecx";
    }
    else{
        return "%edx";
    }
}

void handleArithmetic(LLVMValueRef instruction, unordered_map<LLVMValueRef, int> reg_map, unordered_map<LLVMValueRef, int> offset_map, ofstream& file, string opcode_str){
    // (%a = add nsw A, B)
    LLVMValueRef a = LLVMGetOperand(instruction, 0);
    string reg_a;
    if(reg_map.find(a) != reg_map.end() && reg_map[a] != -1){
        reg_a = mapReg(reg_map[a]);
    }
    else {
        reg_a = "%eax";
    }
    // if a is a constant
    if(LLVMIsConstant(a)){
        int val = LLVMConstIntGetSExtValue(a);
        file << "movl $" + to_string(val) + ", " + reg_a + "\n";
    }
    LLVMValueRef var_A = LLVMGetOperand(instruction, 0);
    LLVMValueRef var_B = LLVMGetOperand(instruction, 1);
    // print the number of operands in the instruction
    
    // instructions for var_A
    if(reg_map.find(var_A) != reg_map.end() && reg_map[var_A] != -1){
        string reg_A = mapReg(reg_map[var_A]);
        // do not emit if both register are the same
        if(reg_A != reg_a){
            file << "movl " + reg_A + ", " + reg_a + "\n";
        }
    }
    else if (isInMemory(var_A)){
        file << "movl " + to_string(offset_map[var_A]) + "(%ebp), " + reg_a + "\n";
    }
    // instructions for var_B
    if(LLVMIsConstant(var_B)){
        long long val = LLVMConstIntGetSExtValue(var_B);
        file << opcode_str + " $" + to_string(val) + ", " + reg_a + "\n";
    }
    else if(reg_map.find(var_B) != reg_map.end() && reg_map[var_B] != -1){
        string reg_B = mapReg(reg_map[var_B]);
        file << opcode_str + reg_B + ", " + reg_a + "\n";
    }
    else if(isInMemory(var_B)){
        file << opcode_str + to_string(offset_map[var_B]) + "(%ebp), " + reg_a + "\n";
    }

    // instruction left for a
    if(isInMemory(a)){
        file << "movl " + reg_a + ", " + to_string(offset_map[a]) + "(%ebp)\n";
    }
}

// main function: code generation
void code_gen(LLVMModuleRef module, char *output, unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> global_reg_map){
    LLVMValueRef func = LLVMGetFirstFunction(module);   // there will be only one function in miniC
    ofstream file(output);

    if (!file.is_open()) {
        std::cout << "Unable to open file";
    }

    printDirectives(file);
    file << "func:\n";
    // get offset
    unordered_map<LLVMValueRef, int> offset_map;
    int localMemSize = initOffsetMap(module, offset_map);
    // print out offset map
    // cout << "printing offset map: \n";
    // for(auto it = offset_map.begin(); it != offset_map.end(); it++){
    //     cout << "key: " << LLVMPrintValueToString(it->first) << " value: " << it->second << "\n";
    // }
    // function prologue
    file <<"pushl %ebp\n";
    file <<"movl %esp, %ebp\n";
    file <<"subl $" + to_string(localMemSize) + ", %esp\n";
    file <<"pushl %ebx\n";
    // loop through each basic block
    unordered_map<LLVMBasicBlockRef, string> bb_labels = createBBLabels(module);
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(func); bb; bb = LLVMGetNextBasicBlock(bb)) {
        // print the basic block label
        file <<bb_labels[bb] + ":\n";
        // get current register map
        unordered_map<LLVMValueRef, int> reg_map = global_reg_map[bb];

        for (LLVMValueRef instruction = LLVMGetFirstInstruction(bb); instruction; instruction = LLVMGetNextInstruction(instruction)) {
            LLVMOpcode opcode = LLVMGetInstructionOpcode(instruction);
            file << "#" <<  LLVMPrintValueToString(instruction) << "\n";
            switch(opcode){
                case LLVMRet:{
                    LLVMValueRef returnValue = LLVMGetOperand(instruction, 0);
                    if(LLVMIsConstant(returnValue)){
                        file <<"movl $" + to_string(LLVMConstIntGetSExtValue(returnValue)) + ", %eax\n";
                    } 
                    else if(isInMemory(returnValue)){
                        // get offset
                        file <<"movl " + to_string(offset_map[returnValue]) + "(%ebp), %eax\n";
                    }
                    else if(isInRegister(returnValue, reg_map)){
                        string reg = mapReg(reg_map[returnValue]);
                        file <<"movl " + reg + ", %eax\n";
                    }
                    else {
                        printf("ERROR: return value not found\n");
                    }
                    break;
                }
                case LLVMLoad:{
                    //printf("LLVMLOAD---------\n");
                    LLVMValueRef dest = LLVMGetOperand(instruction, 0);
                    LLVMValueRef src = LLVMGetOperand(instruction, 1);
                    if(reg_map.find(dest) != reg_map.end() && reg_map[dest] != -1){
                        string reg = mapReg(reg_map[dest]);
                        file << "movl " + to_string(offset_map[src]) + "(%ebp), " + reg + "\n";
                    }
                    // else {
                    //     printf("ERROR: dest not found\n");
                    // }
                    break;
                }
                case LLVMStore:{
                    LLVMValueRef dest = LLVMGetOperand(instruction, 0);
                    LLVMValueRef src = LLVMGetOperand(instruction, 1);
                    if(isParam(dest)){
                        // ignore
                    } 
                    else if(LLVMIsConstant(dest)){
                        // get offset for src
                        int offset = offset_map[src];
                        file <<"movl $" + to_string(LLVMConstIntGetSExtValue(dest)) + ", " + to_string(offset) + "(%ebp)\n";
                    }
                    else if(isInMemory(dest)){
                        // get offset for dest
                        int offset_dest = offset_map[dest];
                        file <<"movl " + to_string(offset_dest) + "(%ebp), %eax\n";
                        int offset_src = offset_map[src];
                        file <<"movl %eax, " + to_string(offset_src) + "(%ebp)\n";
                    }
                    else if(isInRegister(dest, reg_map)){
                        string reg = mapReg(reg_map[dest]);
                        file <<"movl " + reg + ", " + to_string(offset_map[src]) + "(%ebp)\n";
                    }
                    // else {
                    //     printf("ERROR: dest not found\n");
                    // }
                    break;
                }
                case LLVMCall:{
                    file <<"pushl %ecx\n";
                    file <<"pushl %edx\n";
                    if(LLVMGetNumArgOperands(instruction) > 0){
                        // print function
                        LLVMValueRef param = LLVMGetOperand(instruction, 0);
                        if(LLVMIsConstant(param)){
                            file <<"movl $" + to_string(LLVMConstIntGetSExtValue(param)) + ", %eax\n";
                        }
                        else if(isInMemory(param)){
                            file <<"pusl " + to_string(offset_map[param]) + "(%ebp)\n";
                        }
                        else if(isInRegister(param, reg_map)){
                            string reg = mapReg(reg_map[param]);
                            file <<"pushl " + reg + ", %eax\n";
                        }
                        else {
                            printf("ERROR: param not found\n");
                        }
                        file <<"call print\n";
                    }
                    else{
                        file <<"call read\n";
                    }
                    if(LLVMCountParams(instruction) > 0){
                        file <<"addl $4, %esp\n";
                    }
                    if(LLVMGetNumOperands(instruction) > 0){
                        
                        LLVMValueRef dest = LLVMGetOperand(instruction, 0);
                        
                        if(isInMemory(dest)){
                            file <<"movl %eax, " + to_string(offset_map[dest]) + "(%ebp)\n";
                        }
                        else if(isInRegister(dest, reg_map)){
                            string reg = mapReg(reg_map[dest]);
                            file <<"movl %eax, " + reg + "\n";
                        }
                        // else {
                        //     printf("LLVMCALL ERROR: dest not found\n");
                        // }
                    }
                    file <<"popl %edx\n";
                    file <<"popl %ecx\n";
                    break;
                }
                case LLVMBr:{
                    if(LLVMIsConditional(instruction)){
                        LLVMBasicBlockRef trueBB = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 1));
                        LLVMBasicBlockRef falseBB = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 2));
                        
                        LLVMValueRef cond = LLVMGetOperand(instruction, 0);
                        LLVMIntPredicate predicate = LLVMGetICmpPredicate(cond);
                        string jmpInstruction = getAssemblyOpcodeForPredicate(predicate);

                        file << jmpInstruction + bb_labels[trueBB] + "\n";
                        file <<"jmp " + bb_labels[falseBB] + "\n";
                    }
                    else {
                        LLVMBasicBlockRef nextBB = LLVMValueAsBasicBlock(LLVMGetOperand(instruction, 0));
                        file <<"jmp " + bb_labels[nextBB] + "\n";
                    }
                    break;            
                }
                case LLVMAdd:{
                    handleArithmetic(instruction, reg_map, offset_map, file, "addl");
                    break;
                }
                case LLVMSub:{
                    handleArithmetic(instruction, reg_map, offset_map, file, "subl");
                    break;
                }
                case LLVMMul:{
                    handleArithmetic(instruction, reg_map, offset_map, file, "imull");
                    break;
                }
                case LLVMICmp:{
                    // %a = icmp slt A, B
                    LLVMValueRef a = instruction;
                    string reg_a;
                    if(reg_map.find(a) != reg_map.end() && reg_map[a] != -1){
                        reg_a = mapReg(reg_map[a]);
                    }
                    else {
                        reg_a = "%eax";
                    }
                    LLVMValueRef var_A = LLVMGetOperand(instruction, 0);
                    LLVMValueRef var_B = LLVMGetOperand(instruction, 1);
                    // instructions for var_B
                    if(LLVMIsConstant(var_B)){
                        file << "movl $" + to_string(LLVMConstIntGetSExtValue(var_B)) + ", " + reg_a + "\n";
                    }
                    else if(reg_map.find(var_B) != reg_map.end() && reg_map[var_B] != -1){
                        string reg_B = mapReg(reg_map[var_B]);
                        file << "movl " + reg_B + ", " + reg_a + "\n";
                    }
                    else if(isInMemory(var_B)){
                        file << "movl " + to_string(offset_map[var_B]) + "(%ebp), " + reg_a + "\n";
                    }
                    // instructions for var_A
                    if(LLVMIsConstant(var_A)){
                        file << "cmp $" + to_string(LLVMConstIntGetSExtValue(var_A)) + ", " + reg_a + "\n";
                    }
                    else if(isInMemory(var_A)){
                        file << "cmp " + to_string(offset_map[var_A]) + "(%ebp), " + reg_a + "\n";
                    }
                    else if(reg_map.find(var_A) != reg_map.end() && reg_map[var_A] != -1){
                        string reg_A = mapReg(reg_map[var_A]);
                        file << "cmp " + reg_A + ", " + reg_a + "\n";
                    }
                    // deal with a
                   if(isInMemory(a)){
                       file << "movl %eax," + to_string(offset_map[a]) + "(%ebp)\n";
                   }
                   break;
                }
            }
        }
    }
    printFunctionEnd(file);
}