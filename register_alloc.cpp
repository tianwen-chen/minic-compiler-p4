#include "Core.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <set>
#include <list>
#include <cstdlib>

using namespace std;

// subroutines for each basic block: generate inst_index and live_range
unordered_map<LLVMValueRef, int> gen_inst_index(LLVMBasicBlockRef bb) {
  unordered_map<LLVMValueRef, int> inst_index;
  int index = 0;
  for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
    // skip alloc instruction
    
    inst_index[inst] = index;
    index++;
  }
  return inst_index;
}

// helper function to determine if an inst has a lhs
bool has_lhs(LLVMValueRef inst) {
    LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
    if(opcode == LLVMCall){
        // if return type is void, return false, else return true
        LLVMTypeRef ret_type = LLVMTypeOf(inst);
        return (LLVMGetTypeKind(ret_type) != LLVMVoidTypeKind);
    } else if (opcode == LLVMStore || opcode == LLVMBr || opcode == LLVMRet){
        return false;
    } else{
        return true;
    }
}

unordered_map<LLVMValueRef, pair<int, int>> gen_live_range(LLVMBasicBlockRef bb, unordered_map<LLVMValueRef, int> inst_index) {
  unordered_map<LLVMValueRef, pair<int, int>> live_range;
  
  for (LLVMValueRef inst = LLVMGetLastInstruction(bb); inst; inst = LLVMGetPreviousInstruction(inst)) {
    if (LLVMIsAStoreInst(inst)){
        continue;
    }
    // if inst has a lhs, add it to live_map as a key
    if (has_lhs(inst)){
        // get the lhs as the key
        LLVMValueRef lhs = LLVMGetOperand(inst, 0);
        live_range[lhs] = make_pair(inst_index[inst], -1);
    }

    // update the live range of other oprands in the inst
    for (int i = 0; i < LLVMGetNumOperands(inst); i++) {
        LLVMValueRef operand = LLVMGetOperand(inst, i);
        if (live_range.find(operand) != live_range.end()) {
            live_range[operand].second = inst_index[inst];
        }
    }
  }

  return live_range;
}

// helper function: find_spill
LLVMValueRef find_spill(unordered_map<LLVMValueRef, int> reg_map, unordered_map<LLVMValueRef, pair<int, int>> live_range, list<LLVMValueRef> sorted_list, LLVMValueRef inst) {
    // loop over the sorted list
    for (std::list<LLVMValueRef>::iterator it = sorted_list.begin(); it != sorted_list.end(); ++it) {
        // LLVMValueRef it = sorted_list[i];
        if (live_range[*it].first <= live_range[inst].second && live_range[*it].second >= live_range[inst].first) {
            // check if it has a register assigned
            if (reg_map[*it] != -1) {
                return *it;
            }
        }
    }
    return NULL;
}

// main function: register allocation
unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> register_alloc(LLVMModuleRef module){
    // initialize reg_map
    unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> module_reg_map;
    
    // loop through each basic block
    for (LLVMBasicBlockRef bb = LLVMGetFirstBasicBlock(LLVMGetFirstFunction(module)); bb; bb = LLVMGetNextBasicBlock(bb)) {
        unordered_map<LLVMValueRef, int> inst_index = gen_inst_index(bb);
        unordered_map<LLVMValueRef, pair<int, int>> live_range = gen_live_range(bb, inst_index);
        unordered_map<LLVMValueRef, int> reg_map;        
        
        // initialize available register set
        set<int> available_reg = {1, 2, 3};
        // loop through each instruction in the basic block
        for (LLVMValueRef inst = LLVMGetFirstInstruction(bb); inst; inst = LLVMGetNextInstruction(inst)) {
            // printout the avaible_reg
            // for (auto it = available_reg.begin(); it != available_reg.end(); it++) {
            //     cout << *it << " ";
            // }
            // cout << endl;
            
            // print out the current inst
            // char *str = LLVMPrintValueToString(inst);
            // cout << "inst: " << str << endl;
            // ignore if inst is an alloc
            if (LLVMIsAAllocaInst(inst)) {
                continue;
            }
            // if inst is a store, branc, or call, check if the register needs to be freed
            else if (LLVMIsAStoreInst(inst) || LLVMIsABranchInst(inst) || LLVMIsACallInst(inst)) {
                // cout << "store/branch/call" << endl;
                for (auto it = live_range.begin(); it != live_range.end(); it++) {
                    if (reg_map.find(it->first) == reg_map.end()) {
                        continue;
                    }
                    if (it->second.second == inst_index[inst] && reg_map[it->first] != -1) {
                        // add reg to available_reg
                        cout << reg_map[it->first] << " is available and about to be freed" << endl;
                        available_reg.insert(reg_map[it->first]);
                    }
                }
            }
            else {
                LLVMOpcode opcode = LLVMGetInstructionOpcode(inst);
                if (opcode == LLVMAdd || opcode == LLVMSub || opcode == LLVMMul) {
                    LLVMValueRef op1 = LLVMGetOperand(inst, 0);
                    // b) the first operand has a physical register R assigned to it in reg_map
                    if (reg_map.find(op1) != reg_map.end() && reg_map[op1] != -1) {
                        // c) liveness range of the first operand ends at Instr
                        if (live_range[op1].second == inst_index[inst]) {
                            // cout << "add/sub/mul" << endl;
                            reg_map[inst] = reg_map[op1];
                            // check the second operand
                            if (live_range[LLVMGetOperand(inst, 1)].second == inst_index[inst] && reg_map[LLVMGetOperand(inst, 1)] != -1 && reg_map[LLVMGetOperand(inst, 1)] != 0) {
                                available_reg.insert(reg_map[LLVMGetOperand(inst, 1)]);
                            }
                        }
                    }
                } 
                else if (available_reg.size() > 0) {
                    // if a register is available
                    // cout << "available register" << endl;
                    int reg = *available_reg.begin();
                    reg_map[inst] = reg;
                    // delete the reg from available_reg
                    available_reg.erase(reg);
                    // print out available_reg
                    // cout << "available_reg: ";
                    // for (auto it = available_reg.begin(); it != available_reg.end(); it++) {
                    //     cout << *it << " ";
                    // }
                    // cout << endl;
                    // add any reg to available_reg if needs
                    for (auto it = live_range.begin(); it != live_range.end(); it++) {
                        // check if it->first is in reg_map
                        if(reg_map.find(it->first) == reg_map.end()){
                            // if not in reg_map, continue
                            continue;
                        }
                        if (it->second.second == inst_index[inst] && reg_map[it->first] != -1) {
                            int spare_reg = reg_map[it->first];
                            available_reg.insert(spare_reg);
                        }
                    }
                } 
                else {
                    // if a register is not avaible
                    // cout << "no available register" << endl;
                    list<LLVMValueRef> sorted_list;
                    for (auto it = live_range.begin(); it != live_range.end(); it++) {
                        sorted_list.push_back(it->first);
                    }
                    // sort the list based on live range, from the smallest to the largest
                    sorted_list.sort([&live_range](LLVMValueRef a, LLVMValueRef b) {
                        return live_range[a].second > live_range[b].second;
                    });
                    // find a spill
                    LLVMValueRef spill = find_spill(reg_map, live_range, sorted_list, inst);
                    if (spill == NULL){
                        reg_map[inst] = -1;
                    } else {
                        if (live_range[spill].second > live_range[inst].second){
                            reg_map[inst] = -1;
                        } else {
                            // get the reg assigned to spill
                            int spill_reg = reg_map[spill];
                            reg_map[spill] = -1;
                            reg_map[inst] = spill_reg;
                        }
                    }
                    // if the live range of any operand of instr ends and with a reg, free reg
                    for (int i = 0; i < LLVMGetNumOperands(inst); i++) {
                        LLVMValueRef operand = LLVMGetOperand(inst, i);
                        if (live_range[operand].second == inst_index[inst] && reg_map[operand] != -1 && reg_map[operand] != 0) {
                            available_reg.insert(reg_map[operand]);
                        }
                    }
                }
            }
        }
        module_reg_map[bb] = reg_map;
        // // print out the reg_map
        // cout << "reg_map: ";
        // for (auto it = reg_map.begin(); it != reg_map.end(); it++) {
        //     char *str = LLVMPrintValueToString(it->first);
        //     cout << str << " -> " << it->second << " ";
        //     cout << endl;
        // }
        // cout << endl;
        // cout << endl;
    }

    return module_reg_map;
}