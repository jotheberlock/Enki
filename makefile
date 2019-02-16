all: enki objtool

enki: main.o lexer.o ast.o error.o type.o codegen.o asm.o mem.o amd64.o platform.o regset.o pass.o cfuncs.o symbols.o image.o elf.o stringbox.o component.o configfile.o backend.o pe.o entrypoint.o macho.o arm32.o rtti.o thumb.o exports.o inanna.o imports.o 
	g++ -o enki $(LDFLAGS) main.o lexer.o ast.o error.o type.o codegen.o asm.o mem.o amd64.o platform.o regset.o pass.o cfuncs.o symbols.o image.o elf.o stringbox.o component.o configfile.o backend.o pe.o entrypoint.o macho.o arm32.o rtti.o thumb.o exports.o inanna.o imports.o -lpthread

objtool: objtool.o md5.o
	g++ -o objtool objtool.o md5.o

main.o : main.cpp lexer.h ast.h type.h codegen.h
	g++ $(CFLAGS) -g -c -Wall main.cpp

lexer.o : lexer.cpp lexer.h
	g++ $(CFLAGS) -g -c -Wall lexer.cpp

ast.o : ast.cpp ast.h lexer.h codegen.cpp
	g++ $(CFLAGS) -g -c -Wall ast.cpp 

error.o : error.cpp error.h lexer.h
	g++ $(CFLAGS) -g -c -Wall error.cpp

type.o : type.cpp type.h
	g++ $(CFLAGS) -g -c -Wall type.cpp

asm.o : asm.cpp asm.h
	g++ $(CFLAGS) -g -c -Wall asm.cpp

codegen.o : codegen.cpp codegen.h ast.h
	g++ $(CFLAGS) -g -c -Wall codegen.cpp

mem.o : mem.cpp mem.h platform.h
	g++ $(CFLAGS) -g -c -Wall mem.cpp

amd64.o : amd64.cpp amd64.h asm.h
	g++ $(CFLAGS) -g -c -Wall amd64.cpp

platform.o : platform.cpp platform.h
	g++ $(CFLAGS) -g -c -Wall platform.cpp

regset.o : regset.cpp regset.h
	g++ $(CFLAGS) -g -c -Wall regset.cpp

pass.o : pass.cpp pass.h codegen.h
	g++ $(CFLAGS) -g -c -Wall pass.cpp

cfuncs.o : cfuncs.cpp cfuncs.h
	g++ $(CFLAGS) -g -c -Wall cfuncs.cpp

symbols.o : symbols.cpp symbols.h codegen.h
	g++ $(CFLAGS) -g -c -Wall symbols.cpp

image.o : image.cpp image.h mem.h
	g++ $(CFLAGS) -g -c -Wall image.cpp

elf.o : elf.cpp elf.h
	g++ $(CFLAGS) -g -c -Wall elf.cpp

stringbox.o : stringbox.cpp stringbox.h
	g++ $(CFLAGS) -g -c -Wall stringbox.cpp

component.o : component.cpp component.h
	g++ $(CFLAGS) -g -c -Wall component.cpp

configfile.o : configfile.cpp configfile.h
	g++ $(CFLAGS) -g -c -Wall configfile.cpp

backend.o : backend.cpp backend.h
	g++ $(CFLAGS) -g -c -Wall backend.cpp

pe.o : pe.cpp pe.h
	g++ $(CFLAGS) -g -c -Wall pe.cpp

entrypoint.o : entrypoint.cpp entrypoint.h asm.h symbols.h
	g++ $(CFLAGS) -g -c -Wall entrypoint.cpp

macho.o : macho.cpp macho.h
	g++ $(CFLAGS) -g -c -Wall macho.cpp

arm32.o : arm32.cpp arm32.h
	g++ $(CFLAGS) -g -c -Wall arm32.cpp

rtti.o : rtti.cpp rtti.h
	g++ $(CFLAGS) -g -c -Wall rtti.cpp

thumb.o : thumb.cpp thumb.h
	g++ $(CFLAGS) -g -c -Wall thumb.cpp

exports.o : exports.cpp exports.h
	g++ $(CFLAGS) -g -c -Wall exports.cpp

inanna.o : inanna.cpp inanna.h
	g++ $(CFLAGS) -g -c -Wall inanna.cpp

imports.o : imports.cpp imports.h
	g++ $(CFLAGS) -g -c -Wall imports.cpp

objtool.o : tools/objtool.cpp
	g++ $(CFLAGS) -g -c -Wall tools/objtool.cpp

md5.o : tools/md5.cpp
	g++ $(CFLAGS) -g -c -Wall tools/md5.cpp

clean:
	rm -f *.o enki enki.exe *~ objtool

valgrind : enki
	valgrind ./enki tests/TEST.001.twoplustwo.e
