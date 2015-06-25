#include "stringbox.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

StringBox::StringBox()
{
    idp=0;
    internal_idp=-1;
    datap=0;
    data=0;
    ids=0;
}

StringBox::~StringBox()
{
    if(data)
		free(data);
    if(ids)
		free(ids);
}

void StringBox::clear()
{
    if(data)
		free(data);
    if(ids)
		free(ids);
    idp=0;
    internal_idp=-1;
    datap=0;
    data=0;
    ids=0;    
}

char * StringBox::getText(int i)
{
	if(i<0) {
		i*=-1;
		// Awoogah! Memory leak!
		char * buf=(char *)malloc(30);
		sprintf(buf,"<Internal ID %d>",i);
		return buf;
	} else {
		i--;
		if(i==-1 || i>=idp) {
			printf("Eek! Illegal ID\n");
			abort();
			return 0;
		}
		return data+ids[i];
	}
}

int StringBox::getID(const char * c)
{
    for(int loopc=0;loopc<idp;loopc++) {
		if(!strcmp(data+ids[loopc],c))
			return loopc+1;
    }
    return 0;
}

int StringBox::getInternalID()
{
	int ret=internal_idp;
	internal_idp--;
	return ret;
}

int StringBox::add(const char * c)
{
    int t=getID(c);
    if(t)
		return t;
    int l=strlen(c)+1;
    int newsize=datap+l;
    if(!data) {
		data=(char *)malloc(newsize);
    } else {
		data=(char *)realloc(data,newsize);
    }
    if(!ids) {
		ids=(int *)malloc((idp+1)*sizeof(int));
    } else {
		ids=(int *)realloc(ids,(idp+1)*sizeof(int));
    }
    strcpy(data+datap,c);
    ids[idp]=datap;
    int ret=idp;
    datap+=l;
    idp++;
    return ret+1;
}

void StringBox::dump()
{
    printf("IDs %d datasize %d\n\n",idp,datap);
    for(int loopc=0;loopc<idp;loopc++) {
		printf("%d [%s]\n",loopc+1,data+ids[loopc]);
    }
}


/*
int main()
{
StringBox t;
t.add("Hello");
t.add("World!");
t.add("Hello");
t.add("Foo!");
printf("%s\n",t.getText(2));
printf("%d\n",t.getID("Hello"));
t.dump();
}
*/
