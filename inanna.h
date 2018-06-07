#ifndef _INANNA_
#define _INANNA_

/*
Generates Inanna (Enki native) binaries
*/

#include "image.h"
#include "stringbox.h"

class InannaImage : public Image
{
public:

    InannaImage();
    ~InannaImage();
    void finalise();
    bool configure(std::string, std::string);

    virtual uint64 importAddress(std::string)
    {
        return 0;
    }

    virtual uint64 importOffset(std::string)
    {
        return 0;
    }

    std::string name() { return "inanna"; }

protected:

};

#endif
