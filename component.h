#ifndef _COMPONENT_
#define _COMPONENT_

#include <string>
#include <map>

class Component
{
 public:

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
  
  std::map<std::string, ComponentMaker> makers;
  
};

extern ComponentFactory * component_factory;
 
#endif
