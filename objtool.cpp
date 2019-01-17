#include <stdio.h>
#include "inanna_structures.h"

char * strings = 0;

void display_arch(char * ptr, uint32 secs, uint32 relocs)
{
    InannaSection * is = (InannaSection *)ptr;
    for (int loopc=0; loopc<secs; loopc++)
    {
        printf("Type %d offset %d size %d vmem %llx name %s\n",
               is->type, is->offset, is->length, is->vmem, strings+is->name);
        is++;
    }
    printf("\nRelocations:\n");
    InannaReloc * ir = (InannaReloc *)is;
    for (int loopc2=0; loopc2<relocs; loopc2++)
    {
        printf("Type %d from %d:%llx to %d:%llx rshift %d mask %llx lshift %d bits %d offset %d\n",
               ir->type, ir->secfrom, ir->offrom, ir->secto, ir->offto, ir->rshift, ir->mask, ir->lshift,
               ir->bits, ir->offset);
        ir++;
    }
}

int main(int argc, char ** argv)
{
    if (argc != 2)
    {
        printf("Usage: %s <Inanna object file>\n", argv[0]);
        return 0;
    }

    FILE * f = fopen(argv[1], "rb");
    if (!f)
    {
        printf("Can't open %s!\n", argv[1]);
        return 1;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);

    if (len < 512+24)
    {
        printf("Not an Inanna file! Too short\n");
        return 2;
    }
    
    char * buf = new char[len];
    fseek(f, 0, SEEK_SET);
    int read = fread(buf, 1, len, f);
    if (read != len)
    {
        printf("Short read! Expected %d got %d\n", len, read);
        return 3;
    }
    
    fclose(f);
    
    InannaHeader * ih = (InannaHeader *)(buf+INANNA_PREAMBLE);
    if (ih->magic[0] != 'e' || ih->magic[1] != 'n' || ih->magic[2] != 'k' || ih->magic[3] != 'i')
    {
        printf("Not an Inanna file! Wrong magic %c %c %c %c\n",
               ih->magic[0], ih->magic[1], ih->magic[2], ih->magic[3]);
        return 4;
    }

    printf("Version %d, %d architectures, string offset %d (%x), imports offset %d (%x)\n",
           ih->version, ih->archs_count, ih->strings_offset,
           ih->strings_offset, ih->imports_offset, ih->imports_offset);
    strings = buf+ih->strings_offset;
    
    InannaArchHeader * iah = (InannaArchHeader *)(buf+INANNA_PREAMBLE+InannaHeader::size());
    for (int loopc=0; loopc<ih->archs_count; loopc++)
    {
        printf("\nArch %d, start address %llx - sections:\n", iah->arch, iah->start_address);
        display_arch(buf+iah->offset, iah->sec_count, iah->reloc_count);
        iah++;
    }
    
    return 0;
}