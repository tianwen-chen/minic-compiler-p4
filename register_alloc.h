#ifndef REGISTER_ALLOC_H
#define REGISTER_ALLOC_H

#include <iostream>
#include <string>
#include <unordered_map>
#include <set>
#include <list>
#include <llvm-c/Core.h>

using namespace std;

// register allocation function
unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> register_alloc(LLVMModuleRef module);

#endif // REGISTER_ALLOC_H