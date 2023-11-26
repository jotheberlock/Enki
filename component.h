#ifndef _COMPONENT_
#define _COMPONENT_

/*
	A Component is a configurable part of the system - different
	configs can construct different components with different configurations.

	Things like ELF/PE-COFF/etc image generators, compiler passes, machine code
	generators etc are components.
*/

#include <string>
#include <unordered_map>

class Component
{
public:

    virtual ~Component()
    {
    }

	virtual bool configure(std::string, std::string)
	{
		return false;
	}

};

typedef Component * (*ComponentMaker)();

class ComponentFactory
{
public:

	ComponentFactory();
	void add(ComponentMaker, std::string, std::string);
	Component * make(std::string, std::string);

protected:

	std::unordered_map<std::string, ComponentMaker> makers;

};

extern ComponentFactory * component_factory;

#endif
