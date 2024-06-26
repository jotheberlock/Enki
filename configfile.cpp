#include "configfile.h"
#include "asm.h"
#include "component.h"
#include "entrypoint.h"
#include "image.h"
#include "pass.h"
#include <list>
#include <string.h>

Configuration::~Configuration()
{
    for (auto &it : components)
    {
        delete it.second;
    }

    for (auto &it : passes)
    {
        delete it;
    }
}

bool Configuration::lookupConfigConstant(std::string name, uint64_t &val)
{
    auto it = config_constants.find(name);
    if (it == config_constants.end())
    {
        return false;
    }

    val = (*it).second;
    return true;
}

std::string ConfigFile::hostConfig()
{
#ifdef CYGWIN_HOST
    return "cygwin_host.ini";
#endif
#ifdef LINUX_HOST
    return "linux_host.ini";
#endif
#ifdef MACOS_HOST
    return "macos_host.ini";
#endif
#ifdef WINDOWS_HOST
    return "cygwin_host.ini"; // Until something needs changes
#endif
    return "unknown host";
}

std::string ConfigFile::nativeTargetConfig()
{
    char *t = getenv("ENKI_NATIVE_TARGET");
    if (t)
    {
        return t;
    }

#ifdef CYGWIN_HOST
    return "windows_amd64_target.ini";
#endif
#ifdef LINUX_HOST
#ifdef __arm__
    return "linux_arm32_target.ini";
#elif __aarch64__
    return "linux_arm64_target.ini";
#else
    return "linux_amd64_target.ini";
#endif
#endif
#ifdef WINDOWS_HOST
    return "windows_amd64_target.ini";
#endif
#ifdef MACOS_HOST
    return "macos_amd64_target.ini";
#endif
    return "unknown host";
}

std::string ConfigFile::relocatableTargetConfig()
{
#ifdef __arm__
    return "inanna_arm32_target.ini";
#elif __aarch64__
    return "inanna_arm32_target.ini";
#else
    return "inanna_amd64_target.ini";
#endif
}

ConfigFile::ConfigFile(FILE *f, Configuration *c)
{
    file = f;
    config = c;
}

void ConfigFile::addPath(std::string p)
{
    config->paths.push_back(p);
}

void ConfigFile::process()
{
    char buf[4096];
    while (true)
    {
        memset(buf, 0, 4096);
        fgets(buf, 4096, file);
        if (feof(file))
        {
            return;
        }

        char *eol = strstr(buf, "\n");
        if (eol)
        {
            *eol = 0;
        }
        eol = strstr(buf, "\r");
        if (eol)
        {
            *eol = 0;
        }
        if (!processLine(buf))
        {
            printf("Unknown directive %s\n", buf);
        }
    }

    fclose(file);
}

bool ConfigFile::processLine(std::string line)
{
    if (line[0] == '#')
    {
        return true;
    }

    std::string command;
    std::string val;
    if (split(line.c_str(), " ", command, val))
    {
        if (command == "include")
        {
            FILE *f = config->open(val);
            if (f)
            {
                ConfigFile cf(f, config);
                cf.process();
            }
            else
            {
                printf("Can't open include file %s!\n", val.c_str());
            }
        }
        else if (command == "relocatable")
        {
            config->relocatable = (val == "true");
        }
        else if (command == "name")
        {
            config->name = val;
        }
        else if (command == "path")
        {
            config->paths.push_back(val);
        }
        else if (command == "image")
        {
            delete config->image;
            config->image = (Image *)component_factory->make("image", val);
            config->components["image"] = config->image;
        }
        else if (command == "asm")
        {
            delete config->assembler;
            config->assembler = (Assembler *)component_factory->make("asm", val);
            config->components["asm"] = config->assembler;
        }
        else if (command == "cconv")
        {
            delete config->cconv;
            config->cconv = (CallingConvention *)component_factory->make("cconv", val);
            config->components["cconv"] = config->cconv;
        }
        else if (command == "syscall")
        {
            delete config->syscall;
            config->syscall = (CallingConvention *)component_factory->make("cconv", val);
            config->components["syscall"] = config->syscall;
        }
        else if (command == "pass")
        {
            OptimisationPass *op = (OptimisationPass *)component_factory->make("pass", val);
            config->passes.push_back(op);
        }
        else if (command == "entrypoint")
        {
            config->entrypoint = (Entrypoint *)component_factory->make("entrypoint", val);
            config->components["entrypoint"] = config->entrypoint;
        }
        else if (command == "file")
        {
            config->preloads.push_back(val);
        }
        else if (command == "set")
        {
            std::string cname;
            std::string setter;
            if (!split(val, ".", cname, setter))
            {
                printf("Invalid param syntax %s\n", val.c_str());
                return false;
            }

            std::string param;
            std::string paramval;
            if (!split(setter, "=", param, paramval))
            {
                printf("Invalid param syntax %s\n", setter.c_str());
                return false;
            }
            Component *c = config->components[cname];
            if (!c)
            {
                printf("Couldn't find object %s\n", cname.c_str());
            }
            else if (!c->configure(param, paramval))
            {
                printf("Couldn't set %s to %s on %s\n", param.c_str(), paramval.c_str(), cname.c_str());
            }
        }
        else if (command == "constant")
        {
            std::string name;
            std::string value;
            if (!split(val, " ", name, value))
            {
                printf("Invalid constant syntax %s\n", val.c_str());
                return false;
            }
            uint64_t nval = strtol(value.c_str(), 0, 0);
            config->config_constants[name] = nval;
        }
        else
        {
            return false;
        }
    }
    return true;
}

bool ConfigFile::split(std::string i, std::string s, std::string &o1, std::string &o2)
{
    size_t pos = i.find(s);
    if (pos == std::string::npos)
    {
        return false;
    }

    o1 = i.substr(0, pos);
    o2 = i.substr(pos + 1, std::string::npos);

    return true;
}

FILE *Configuration::open(std::string n)
{
    FILE *f = fopen(n.c_str(), "rb");
    if (f)
    {
        return f;
    }

    for (auto &it: paths)
    {
        std::string path = it + n;
        FILE *f = fopen(path.c_str(), "rb");
        if (f)
        {
            return f;
        }
    }
    return 0;
}
