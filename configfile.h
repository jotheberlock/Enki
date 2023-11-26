#ifndef _CONFIGFILE_
#define _CONFIGFILE_

/*
	Handles parsing .ini files and generating Configurations, which
	hold the necessary Components and configs to generate e.g.
	'a Linux ARM32 binary'.
*/

#include <stdio.h>
#include <string>
#include <list>
#include <map>
#include <vector>

#include "platform.h"

class Image;
class Component;
class CallingConvention;
class OptimisationPass;
class Assembler;
class Entrypoint;

class Configuration
{
public:

	Configuration()
	{
		image = 0;
		cconv = 0;
		syscall = 0;
		assembler = 0;
		entrypoint = 0;
        relocatable = false;
	}

    ~Configuration();
    
	FILE * open(std::string);
    
        // Config can define constants e.g. IS_WINDOWS, SYSCALL_WRITE
    bool lookupConfigConstant(std::string, uint64_t &);
    
	std::list<std::string> paths;
	std::map<std::string, Component *> components;
	std::vector<std::string> preloads;
	Image * image;
	CallingConvention * cconv;
	CallingConvention * syscall;
	Assembler * assembler;
	Entrypoint * entrypoint;
	std::string name;
	std::vector<OptimisationPass *> passes;
    std::map<std::string, uint64_t> config_constants;
    bool relocatable;
        
};

class ConfigFile
{

public:

	ConfigFile(FILE *, Configuration *);
	void process();
	bool processLine(std::string line);

    static std::string hostConfig();
	static std::string nativeTargetConfig();
    static std::string relocatableTargetConfig();
    
	bool split(std::string, std::string, std::string &, std::string &);
	void addPath(std::string);

protected:

	FILE * file;
	Configuration * config;

};

extern Configuration * configuration;

#endif
