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

class Configuration
{
 public:

    Configuration()
    {
        image = 0;
	cconv = 0;
	syscall = 0;
	assembler = 0;
    }
  
    std::list<std::string> paths;
    std::map<std::string, Component *> components;
    Image * image;
    CallingConvention * cconv;
    CallingConvention * syscall;
    Assembler * assembler;
    std::string name;
    
    std::vector<OptimisationPass *> passes;
    
};

class ConfigFile
{

 public:

  ConfigFile(FILE *, Configuration *);
  void process();
  static std::string hostConfig();
  
  bool split(std::string, std::string, std::string &, std::string &);
  FILE * open(std::string);
  void addPath(std::string);
  
 protected:

  FILE * file;
  Configuration * config;
  
};

#endif
