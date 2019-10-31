#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <map>

#include "../inanna_structures.h"
#include "md5.h"

char * strings = 0;

const char * arch_names [] = 
{
    "unknown",
    "amd64",
    "arm32",
    "thumb" 
};

const char * reloc_types[] =
{
    "invalid",
    "reloc64",
    "reloc32",
    "reloc16",
    "relocmasked64",
    "relocmasked32",
    "relocmasked16"
};

int round_to_4k(int size)
{
    return (size+4096-1) & ~4096;
}

void display_arch(char * ptr, uint32 secs, uint32 relocs)
{
    InannaSection * is = (InannaSection *)ptr;
    for (unsigned int loopc=0; loopc<secs; loopc++)
    {
        printf("Type %d offset %d size %d vmem %llx name %s\n",
               is->type, is->offset, is->length, is->vmem, strings+is->name);
        is++;
    }
    printf("\nRelocations:\n");
    InannaReloc * ir = (InannaReloc *)is;
    for (unsigned int loopc2=0; loopc2<relocs; loopc2++)
    {
        printf("Type %s, from %d:%llx to %d:%llx, rshift %d mask %llx lshift %d bits %d offset %lld\n",
               ir->type < INANNA_RELOC_END ? reloc_types[ir->type] :
               "<too big!>", ir->secfrom, ir->offrom, ir->secto, ir->offto,
               ir->rshift, ir->mask, ir->lshift, ir->bits, ir->offset);
        ir++;
    }
}

class ArchContents
{
public:

    InannaArchHeader * iah;
    std::vector<InannaSection *> secs;
    std::vector<std::string> md5s;
    std::vector<InannaReloc *> relocs;
    char* imports;
    std::string imports_md5;

};

class FileContents
{
public:

    FileContents() { data=0; ih=0; }
    ~FileContents() { delete[] data; }
    
    bool load(std::string);
    void build(std::string);
    
    void print();

    int find_arch(uint32);
    
    std::string name;
    char * data;
    int len;
    InannaHeader * ih;
    std::vector<ArchContents> archs;
    char* strings;
};

int FileContents::find_arch(uint32 a)
{
    for (unsigned int loopc=0; loopc<archs.size(); loopc++)
    {
        if (archs[loopc].iah->arch == a)
        {
            return loopc;
        }
    }

    return -1;
}

std::string do_md5(char * ptr, int len)
{
    MD5_CTX ctx;
    MD5_Init(&ctx);
    MD5_Update(&ctx, ptr, len);
    unsigned char buf[MD5_DIGEST_LENGTH];
    memset(buf, 0, MD5_DIGEST_LENGTH);
    MD5_Final(buf, &ctx);
    std::string md5;
    for (int loopc4=0; loopc4<MD5_DIGEST_LENGTH; loopc4++)
    {
        char hex[3];
        sprintf(hex, "%02x", buf[loopc4]);
        md5 += hex;
    }
    return md5;
}

void FileContents::print()
{
    printf("File: %s - version %d, %d %s, string offset %d (%x) size %d, imports offset %d (%x) size %d\n",
           name.c_str(), ih->version, ih->archs_count, ih->archs_count == 1 ?
           "architecture" : "architectures", ih->strings_offset,
           ih->strings_offset, ih->strings_size, ih->imports_offset, ih->imports_offset,
           ih->imports_size);

    uint64 * import_modulesp = (uint64 *)(data+ih->imports_offset);
    printf("Imports %lld %s\n", *import_modulesp,
           *import_modulesp == 1 ? "module" : "modules");
    
    for (unsigned int loopc=0; loopc<archs.size(); loopc++)
    {
        InannaArchHeader * i = archs[loopc].iah;
        printf("  Arch %s (%d), start address %llx\n",
               i->arch > 3 ? "<invalid!>" : arch_names[i->arch], i->arch,
               i->start_address);
        printf("  %d sections\n", i->sec_count);
        for (unsigned int loopc2=0; loopc2<i->sec_count; loopc2++)
        {
            InannaSection * is = archs[loopc].secs[loopc2];
            printf("    Type %d offset %d size %d vmem %llx name %s md5 %s\n",
                   is->type, is->offset, is->length, is->vmem,
                   strings+is->name, archs[loopc].md5s[loopc2].c_str());
        }
        printf("  %d relocations\n", i->reloc_count);
        for (unsigned int loopc3=0; loopc3<i->reloc_count; loopc3++)
        {
            InannaReloc * ir = archs[loopc].relocs[loopc3];
            printf("    Type %s, from %d:%llx to %d:%llx, rshift %d mask %llx lshift %d bits %d offset %lld\n",
                   ir->type < INANNA_RELOC_END ? reloc_types[ir->type] :
                   "<too big!>", ir->secfrom, ir->offrom, ir->secto, ir->offto,
                   ir->rshift, ir->mask, ir->lshift, ir->bits, ir->offset);
        }
    }
    
    printf("\n");
}

std::map<std::string, char *> sections_by_md5;
         
bool FileContents::load(std::string n)
{
    name = n;
    FILE * f = fopen(name.c_str(), "rb");
    if (!f)
    {
        printf("Can't open %s!\n", name.c_str());
        return false;
    }

    fseek(f, 0, SEEK_END);
    long len = ftell(f);

    if (len < 512+24)
    {
        printf("Not an Inanna file! Too short, length %ld\n", len);
        fclose(f);
        return false;
    }
    
    data = new char[len];

    fseek(f, 0, SEEK_SET);
    int read = fread(data, 1, len, f);
    if (read != len)
    {
        printf("Short read! Expected %ld, got %d\n", len, read);
        delete[] data;
        fclose(f);
        return false;
    }

    ih = (InannaHeader *)(data+INANNA_PREAMBLE);
    
    if (ih->magic[0] != 'e' || ih->magic[1] != 'n' || ih->magic[2] != 'k' || ih->magic[3] != 'i')
    {
        printf("Not an Inanna file! Wrong magic [%c%c%c%c]\n",
               ih->magic[0], ih->magic[1], ih->magic[2], ih->magic[3]);
        delete[] data;
        return false;
    }

    if (ih->imports_offset > len)
    {
        printf("File length %ld too short for imports offset of %d!\n",
               len, ih->imports_offset);
        delete[] data;
        return false;
    }

    if (ih->strings_offset > len)
    {
        printf("File length %ld too short for strings offset %d!\n",
               len, ih->strings_offset);
        delete[] data;
        return false;
    }
    
    strings = data + ih->strings_offset;
    
    InannaArchHeader * iahp = (InannaArchHeader *)(data+INANNA_PREAMBLE+InannaHeader::size());
    for (unsigned int loopc=0; loopc<ih->archs_count; loopc++)
    {
        ArchContents ac;
        ac.iah = iahp;

        InannaSection * is = (InannaSection *)(data+iahp->offset);
        for (unsigned int loopc2=0; loopc2<iahp->sec_count; loopc2++)
        {
            ac.secs.push_back(is);
            std::string md5 = do_md5(data+is->offset, is->length);
            ac.md5s.push_back(md5);
            sections_by_md5[md5] = data+is->offset;
            is++;
        }
        InannaReloc * ir = (InannaReloc *)is;
        for (unsigned int loopc3=0; loopc3<iahp->reloc_count; loopc3++)
        {
            ac.relocs.push_back(ir);
            ir++;
        }
        
        ac.strings = data + iahp->imports_offset;
        ac.imports_md5 = do_md5(ac.strings, iahp->imports_size);
        archs.push_back(ac);
        iahp++;
    }
    
    fclose(f);
    return true;
}

std::vector<FileContents *> sources;

void FileContents::build(std::string n)
{
    name = n;
    ih = new InannaHeader;
    char * fstrings_ptr = 0;
    char * fimports_ptr = 0;
    int fstrings_size = 0;
    int fimports_size = 0;
    
    for (unsigned int loopc=0; loopc<sources.size(); loopc++)
    {
        FileContents * fc = sources[loopc];

        if (loopc ==0)
        {
            fstrings_ptr= fc->data + fc->ih->strings_offset;
            fstrings_size = fc->ih->strings_size;
            fimports_ptr = fc->data + fc->ih->imports_offset;
            fimports_size = fc->ih->imports_size;
        }
        else
        {
            char * thisstrings_ptr = fc->data + fc->ih->strings_offset;
            //char * thisimports_ptr = fc->data + fc->ih->strings_offset;
            if (fc->ih->strings_size != fstrings_size ||
                memcmp(fstrings_ptr, thisstrings_ptr, fstrings_size) != 0)
            {
                printf("Ignoring mismatched strings section!\n");
                continue;
            }
            
            /*
            if (fc->ih->imports_size != fimports_size ||
                memcmp(fimports_ptr, thisimports_ptr, fimports_size) != 0)
            {
                printf("Ignoring mismatched imports section in %s! %d %d %s %s\n",
                       fc->name.c_str(),
                       fc->ih->imports_size, fimports_size,
                       do_md5(fimports_ptr, fimports_size).c_str(),
                       do_md5(thisimports_ptr, fc->ih->imports_size).c_str());
                continue;
            }
            else
            {
                printf("%s matches\n", fc->name.c_str());
            } 
            */
        }
        
        for (unsigned int loopc2=0; loopc2<fc->archs.size(); loopc2++)
        {
            ArchContents & ac = fc->archs[loopc2];
            if (find_arch(ac.iah->arch) > -1)
            {
                printf("Ignoring duplicate arch %s in %s!\n", arch_names[ac.iah->arch], fc->name.c_str());
                break;
            }
        }
    }
}

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        printf("Usage: %s [-c combined file] <Inanna object file> <Inanna object file> ...\n", argv[0]);
        return 0;
    }

    bool making_combined = false;
    const char * combined_output = "";
    int idx = 1;
    
    if (!strcmp(argv[1], "-c"))
    {
        if (argc < 4)
        {
            printf("Must specify combined output file and at least one input file!\n");
            return 1;
        }
        
        making_combined = true;
        combined_output = argv[2];
        idx = 3;
    }

    for (int loopc=idx; loopc<argc; loopc++)
    {
        FileContents * fc = new FileContents;
        if (fc->load(argv[loopc]))
        {
            if (making_combined)
            {
                sources.push_back(fc);
            }
            else
            {
                fc->print();
                delete fc;
            }
        }
        else
        {
            delete fc;
        }
    }

    if (making_combined)
    {
        FileContents output;
        output.build(combined_output);
    }
    
    for (unsigned int loopc=0; loopc<sources.size(); loopc++)
    {
        delete sources[loopc];
    }

    return 0;
}
