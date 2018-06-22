#include "inanna.h"
#include "platform.h"
#include "symbols.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#if defined(POSIX_HOST)
#include <sys/stat.h>
#endif

class InannaSection
{
public:

    uint32 arch;
    uint32 type;
    uint32 offset;
    uint32 size;
    
};

InannaImage::InannaImage()
{
    arch = 0;
    for (int loopc=0; loopc<IMAGE_LAST; loopc++)
    {
        bases[loopc] = 0;
    }   
}

InannaImage::~InannaImage()
{
}

void InannaImage::finalise()
{
    FILE * f = fopen(fname.c_str(), "wb+");
    if (!f)
    {
        printf("Can't open %s\n", fname.c_str());
        return;
    }

    unsigned char * header = new unsigned char[4096];
    memset((char *)header, 0, 4096);
    strcpy((char *)header, "/usr/bin/env enki\n");
    strcpy((char *)header + 512, "enki");

    fwrite(header, 4096, 1, f);

    fclose(f);
#if defined(POSIX_HOST)
    chmod(fname.c_str(), 0755);
#endif
}

bool InannaImage::configure(std::string param, std::string val)
{
    if (false)
    {

    }
    else
    {
        return Image::configure(param, val);
    }

    return true;
}
