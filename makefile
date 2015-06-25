parsey: main.o lexer.o ast.o error.o type.o codegen.o asm.o mem.o amd64.o platform.o regset.o pass.o cfuncs.o symbols.o image.o elf.o stringbox.o
	g++ -o parse main.o lexer.o ast.o error.o type.o codegen.o asm.o mem.o amd64.o platform.o regset.o pass.o cfuncs.o symbols.o image.o elf.o stringbox.o -lpthread

main.o : main.cpp lexer.h ast.h type.h codegen.h
	g++ -g -c -Wall main.cpp

lexer.o : lexer.cpp lexer.h
	g++ -g -c -Wall lexer.cpp

ast.o : ast.cpp ast.h lexer.h codegen.cpp
	g++ -g -c -Wall ast.cpp 

error.o : error.cpp error.h lexer.h
	g++ -g -c -Wall error.cpp

type.o : type.cpp type.h
	g++ -g -c -Wall type.cpp

asm.o : asm.cpp asm.h
	g++ -g -c -Wall asm.cpp

codegen.o : codegen.cpp codegen.h ast.h
	g++ -g -c -Wall codegen.cpp

mem.o : mem.cpp mem.h platform.h
	g++ -g -c -Wall mem.cpp

amd64.o : amd64.cpp amd64.h asm.h
	g++ -g -c -Wall amd64.cpp

platform.o : platform.cpp platform.h
	g++ -g -c -Wall platform.cpp

regset.o : regset.cpp regset.h
	g++ -g -c -Wall regset.cpp

pass.o : pass.cpp pass.h codegen.h
	g++ -g -c -Wall pass.cpp

cfuncs.o : cfuncs.cpp cfuncs.h
	g++ -g -c -Wall cfuncs.cpp

symbols.o : symbols.cpp symbols.h codegen.h
	g++ -g -c -Wall symbols.cpp

image.o : image.cpp image.h mem.h
	g++ -g -c -Wall image.cpp

elf.o : elf.cpp elf.h
	g++ -g -c -Wall elf.cpp

stringbox.o : stringbox.cpp stringbox.h
	g++ -g -c -Wall stringbox.cpp

clean:
	rm *.o parsey parsey.exe *~

