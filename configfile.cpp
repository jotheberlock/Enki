#include "configfile.h"
#include <list>
#include <string.h>

static std::list<std::string> paths;

ConfigFile::ConfigFile(FILE * f)
{
    file = f;
}

void ConfigFile::addPath(std::string p)
{
    paths.push_back(p);
}

void ConfigFile::process()
{
    char buf[4096];
    while (true)
    {
        fgets(buf, 4096,file);
	if (feof(file))
	{
	    return;
	}
	  
	char * eol = strstr(buf, "\n");
	if (eol)
        {
	    *eol = 0;
	}
	eol = strstr(buf, "\r");
	if (eol)
	{
	    *eol = 0;
        }
	
	if (buf[0] == '#')
	{
	    continue;
        }

	std::string command;
	std::string val;
	if (split(buf, " ", command, val))
	{
	    if (command == "include")
	    {
	        FILE * f = open(val);
		if (f)
		{
  		    ConfigFile cf(f);
		    cf.process();
		}
		else
		{
  		    printf("Can't include %s!\n", val.c_str());
		}
	    }
	    else if (command == "path")
	    {
		paths.push_back(val);
	    }
	    else
	    {
  	        printf("Unknown directive %s\n", command.c_str());
	    }
	}
    }

    fclose(file);
}

bool ConfigFile::split(std::string i, std::string s, std::string & o1,
		       std::string & o2)
{
    unsigned int pos = i.find(s);
    if (pos == std::string::npos)
    {
        return false;
    }

    o1 = i.substr(0, pos);
    o2 = i.substr(pos+1, std::string::npos);

    return true;
}

FILE * ConfigFile::open(std::string n)
{
    for (std::list<std::string>::iterator it = paths.begin();
         it != paths.end(); it++)
    {
        std::string path = *it + n;
	FILE * f = fopen(path.c_str(), "r");
	if (f)
	{
	    return f;
	}
    }

    return 0;
}
