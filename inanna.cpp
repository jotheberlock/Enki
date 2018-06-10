#include "inanna.h"

InannaImage::InannaImage()
{
    arch = 0;
}

InannaImage::~InannaImage()
{
}

void InannaImage::finalise()
{
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