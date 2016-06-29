#ifndef _CONFIGFILE_
#define _CONFIGFILE_

#include <stdio.h>
#include <string>
#include <list>
#include <map>
#include <vector>

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
	}

	FILE * open(std::string);

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

};

class ConfigFile
{

public:

	ConfigFile(FILE *, Configuration *);
	void process();
	bool processLine(std::string line);
	static std::string hostConfig();
	static std::string nativeTargetConfig();

	bool split(std::string, std::string, std::string &, std::string &);
	void addPath(std::string);

protected:

	FILE * file;
	Configuration * config;

};

extern Configuration * configuration;

#endif
