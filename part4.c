#include <stdio.h>
#include <stdlib.h>
#include "register_alloc.h"
#include "code_gen.h"
#include <llvm-c/Core.h>
#include <llvm-c/IRReader.h>
#include <llvm-c/Types.h>

using namespace std;

LLVMModuleRef createLLVMModel(char * filename){
	char *err = 0;

	LLVMMemoryBufferRef ll_f = 0;
	LLVMModuleRef m = 0;

	LLVMCreateMemoryBufferWithContentsOfFile(filename, &ll_f, &err);

	if (err != NULL) { 
		printf("Error: %s\n", err);
		return NULL;
	}
	
	LLVMParseIRInContext(LLVMGetGlobalContext(), ll_f, &m, &err);

	if (err != NULL) {
		printf("Error: %s\n", err);
	}

	return m;
}

/*
    input: src file (ll file), dest file (s file)
    generate llvm module from the ll file and print out the assembly code to the s file
*/
int main(int argc, char *argv[]){
    char* input = argv[1];
    char* output = argv[2];
    LLVMModuleRef module = createLLVMModel(input);
    // test: register allocation
    unordered_map<LLVMBasicBlockRef, unordered_map<LLVMValueRef, int>> reg_map = register_alloc(module);
    // print out the number of basic blocks in module
    // print out the reg_map, use llvmdumpvalue to print key
    for (auto it = reg_map.begin(); it != reg_map.end(); ++it) {
        printf("basic block\n");
        for (auto it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
            // use printvaluetostring
            char *str = LLVMPrintValueToString(it2->first);
            printf("key: %s, value: %d\n", str, it2->second);
        }
    }
    // test: code generation
    code_gen(module, output, reg_map);
    return 0;
}