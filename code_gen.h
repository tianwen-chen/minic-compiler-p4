#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>
#include <string.h>

void code_gen(LLVMModuleRef module, char *outfile, unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> global_reg_map);

#endif // CODE_GEN_H