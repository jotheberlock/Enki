#ifndef _CONFIGFILE_
#define _CONFIGFILE_

#include <stdio.h>
#include <string>

class ConfigFile
{

 public:

  ConfigFile(FILE *);
  void process();

  bool split(std::string, std::string, std::string &, std::string &);
  FILE * open(std::string);
  void addPath(std::string);
  
 protected:

  FILE * file;
  
};

#endif
