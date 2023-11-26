cmake_minimum_required(VERSION 3.10)
project(Enki VERSION 1.0
             DESCRIPTION "Enki"
             LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_executable(enki
	amd64.cpp
	arm32.cpp
	asm.cpp
	ast.cpp
	backend.cpp
	cfuncs.cpp
	codegen.cpp
	component.cpp
	configfile.cpp
	elf.cpp
	entrypoint.cpp
	error.cpp
	exports.cpp
	image.cpp
	imports.cpp
	inanna.cpp
	lexer.cpp
	macho.cpp
	main.cpp
	mem.cpp
	pass.cpp
	pe.cpp
	platform.cpp
	regset.cpp
	rtti.cpp
	stringbox.cpp
	symbols.cpp
	thumb.cpp
	type.cpp
	amd64.h
	arm32.h
	asm.h
	ast.h
	backend.h
	cfuncs.h
	codegen.h
	component.h
	configfile.h
	elf.h
	entrypoint.h
	error.h
	exports.h
	image.h
	imports.h
	inanna.h
	inanna_structures.h
	lexer.h
	macho.h
	mem.h
	pass.h
	pe.h
	platform.h
	regset.h
	rtti.h
	stringbox.h
	symbols.h
	thumb.h
	type.h)

add_executable(objtool
	tools/md5.h
	tools/md5.cpp
	tools/objtool.cpp)

add_definitions(-D_CRT_SECURE_NO_WARNINGS=1)  # Don't complain about6 e.g. fopen() and strcpy() in Visual Studio
