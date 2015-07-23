#ifndef _CONFIGFILE_
#define _CONFIGFILE_

#include <stdio.h>
#include <string>
#include <list>
#include <map>

class Image;
class Component;

class Configuration
{
 public:

    Configuration()
    {
        image = 0;
    }
  
    std::list<std::string> paths;
    std::map<std::string, Component *> components;
    Image * image;
  
};

class ConfigFile
{

 public:

  ConfigFile(FILE *, Configuration *);
  void process();

  bool split(std::string, std::string, std::string &, std::string &);
  FILE * open(std::string);
  void addPath(std::string);
  
 protected:

  FILE * file;
  Configuration * config;
  
};

#endif
