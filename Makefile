MAIN = part4
CODEGEN = code_gen
REGALLOC = register_alloc

$(MAIN): $(MAIN).c $(CODEGEN).cpp $(REGALLOC).cpp
	g++ -g -I /usr/include/llvm-c-15/ -c $(MAIN).c $(CODEGEN).cpp $(REGALLOC).cpp
	g++ $(MAIN).o $(CODEGEN).o $(REGALLOC).o `llvm-config-15 --cxxflags --ldflags --libs core` -I /usr/include/llvm-c-15/ -o $@

$(CODEGEN).o: $(CODEGEN).cpp $(CODEGEN).h
	g++ -g -I /usr/include/llvm-c-15/ -c $(CODEGEN).cpp

$(REGALLOC).o: $(REGALLOC).cpp $(REGALLOC).h
	g++ -g -I /usr/include/llvm-c-15/ -c $(REGALLOC).cpp

clean: 
	rm -rf $(MAIN)
	rm -rf *.o
	rm -rf test.s