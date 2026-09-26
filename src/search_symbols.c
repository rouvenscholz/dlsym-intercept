#include <elf.h>
#include <libelf.h>
#include <link.h>
#include <stdio.h>
#include <string.h>
#include "search_symbols.h"


#if __SIZEOF_POINTER__ == 8
#define ELF_R_SYM(info)     ELF64_R_SYM(info)
#define ELF_R_TYPE(info)    ELF64_R_TYPE(info)
#define ELF_ST_TYPE(info)   ELF64_ST_TYPE(info)
#define ELF_ST_BIND(info)   ELF64_ST_BIND(info)
#define ELFCLASS_BITS 64
#else
#define ELF_R_SYM(info)     ELF32_R_SYM(info)
#define ELF_R_TYPE(info)    ELF32_R_TYPE(info)
#define ELF_ST_TYPE(info)   ELF32_ST_TYPE(info)
#define ELF_ST_BIND(info)   ELF32_ST_BIND(info)
#define ELFCLASS_BITS 32
#endif


#ifdef DEBUG
#define IFDEBUG
#else
#define IFDEBUG if(0)
#endif

static uint32_t _dl_new_hash (const char *s)
{
   uint32_t h = 5381;
   for (unsigned char c = *s; c != '\0'; c = *++s)
     h = h * 33 + c;
   return h;
}



static int parse_gnu_hash_tbl(ElfW(Addr) baseaddr, search_symbols_t* search_symbols, ElfW(Word)* dt_gnu_hash, ElfW(Sym)* dt_symtab, const char* dt_strtab)
{
    const ElfW(Word)    nbuckets =      dt_gnu_hash[0];
    IFDEBUG printf("parse_gnu_hash_tbl() nbuckets %u\n", nbuckets);
    if (nbuckets == 0)
    {
        return 0;
    }
    const ElfW(Word)    symoffset =     dt_gnu_hash[1];
    const ElfW(Word)    bloom_size =    dt_gnu_hash[2];
    const ElfW(Word)    bloom_shift =   dt_gnu_hash[3];
    const ElfW(Addr)*   bloom =         (ElfW(Addr)*)(dt_gnu_hash + 4);
    const ElfW(Word)*   buckets =       (ElfW(Word)*)(bloom + bloom_size);
    const ElfW(Word)*   chain =         buckets + nbuckets;
    int all_symbols_found = 1;

    for(search_symbol_entry_t* curr = search_symbols->search_symbols_entries; curr < search_symbols->search_symbols_entries + search_symbols->search_symbols_entries_num; curr++)
    {
        if (!*curr->reloc_ptr)
        {
            if (!search_symbols->gnu_hashes_valid)
            {
                curr->symbol_gnu_hash = _dl_new_hash(curr->symbol_name);
            }
            IFDEBUG printf("search for %s\n", curr->symbol_name);
            const ElfW(Addr) word = bloom[(curr->symbol_gnu_hash / ELFCLASS_BITS) % bloom_size];
            const ElfW(Addr) mask = 0
                | (ElfW(Addr))1 << (curr->symbol_gnu_hash % ELFCLASS_BITS)
                | (ElfW(Addr))1 << ((curr->symbol_gnu_hash >> bloom_shift) % ELFCLASS_BITS);

            /* If at least one bit is not set, a symbol is surely missing. */
            if ((word & mask) != mask)
            {
                IFDEBUG printf("(word & mask) != mask\n");
                all_symbols_found = 0;
                continue;
            }
            Elf_Symndx symix = buckets[curr->symbol_gnu_hash % nbuckets];
            if (symix < symoffset)
            {
                all_symbols_found = 0;
                continue;
            }

            /* Loop through the chain. */
            while (1)
            {
                const ElfW(Sym)* symbol = dt_symtab + symix;
                const char* symname = dt_strtab + symbol->st_name;
                const ElfW(Word) hash = chain[symix - symoffset];
                if ((curr->symbol_gnu_hash|1) == (hash|1) && strcmp(curr->symbol_name, symname) == 0)
                {
                    void* addr = (void*)(baseaddr + symbol->st_value);
                    uint32_t st_type = ELF_ST_TYPE(symbol->st_info);
                    uint32_t st_bind = ELF_ST_BIND(symbol->st_info);
#ifdef DEBUG
                    #define X(name) if (st_type == name) printf("parse_gnu_hash_tbl found %s at %p, st_info %s", curr->symbol_name, addr, #name);
                        X(STT_NOTYPE) X(STT_OBJECT) X(STT_FUNC) X(STT_SECTION) X(STT_FILE) X(STT_COMMON) X(STT_TLS)  \
                        X(STT_NUM) X(STT_LOOS) X(STT_GNU_IFUNC) X(STT_HIOS) X(STT_LOPROC) X(STT_HIPROC) \
                        X(STT_SPARC_REGISTER) X(STT_PARISC_MILLICODE) X(STT_HP_OPAQUE) X(STT_HP_STUB) X(STT_ARM_TFUNC) X(STT_ARM_16BIT)
                    #undef X
                    #define X(name) if (st_bind == name) printf(", binding %s\n", #name);
                        X(STB_LOCAL) X(STB_GLOBAL) X(STB_WEAK) X(STB_NUM) X(STB_LOOS) X(STB_GNU_UNIQUE) X(STB_HIOS) X(STB_LOPROC) \
                        X(STB_HIPROC) X(STB_MIPS_SPLIT_COMMON)
                    #undef X
#endif
                    if (st_type == STT_FUNC && st_bind == STB_GLOBAL)
                    {
                        *curr->reloc_ptr = addr;
                    }
                    else
                    {
                        all_symbols_found = 0;
                    }
                    break;
                }

                /* Chain ends with an element with the lowest bit set to 1. */
                if (hash & 1)
                {
                    all_symbols_found = 0;
                    break;
                }
                IFDEBUG printf("parse_gnu_hash_tbl search %s hash collision with %s\n", curr->symbol_name, dt_strtab + symbol->st_name);
                symix++;
            }
        }
    }
    search_symbols->gnu_hashes_valid = 1;
    return all_symbols_found;
}



static int parse_hash_tbl(ElfW(Addr) baseaddr, search_symbols_t* search_symbols, ElfW(Word)* dt_hash, ElfW(Sym)* dt_symtab, const char* dt_strtab)
{
    const ElfW(Word) nbucket = dt_hash[0];
    if (nbucket == 0)
    {
        return 0;
    }
    const ElfW(Word) nchain  = dt_hash[1];
    (void)nchain;
    const ElfW(Word)* bucket = dt_hash + 2;
    const ElfW(Word)* chain =  dt_hash + (nbucket + 2);
    int all_symbols_found = 1;

    for(search_symbol_entry_t* curr = search_symbols->search_symbols_entries; curr < search_symbols->search_symbols_entries + search_symbols->search_symbols_entries_num; curr++)
    {
        if (!*curr->reloc_ptr)
        {
            if (!search_symbols->hashes_valid)
            {
                curr->symbol_hash = elf_hash(curr->symbol_name);
            }
            IFDEBUG printf("search for: %s\n", curr->symbol_name);
            ElfW(Word) i = bucket[curr->symbol_hash % nbucket];
            for (; bucket != STN_UNDEF; i = chain[i])
            {
                ElfW(Sym)* symbol = dt_symtab + i;
                if (strcmp(curr->symbol_name, dt_strtab + symbol->st_name) == 0)
                {
                    uint32_t st_type = ELF_ST_TYPE(symbol->st_info);
                    uint32_t st_bind = ELF_ST_BIND(symbol->st_info);
                    if (st_type == STT_FUNC && st_bind == STB_GLOBAL)
                    {
                        void* addr = (void*)(baseaddr + symbol->st_value);
#ifdef DEBUG
                        #define X(name) if (st_type == name) printf("parse_hash_tbl found %s at %p, st_info %s", curr->symbol_name, addr, #name);
                            X(STT_NOTYPE) X(STT_OBJECT) X(STT_FUNC) X(STT_SECTION) X(STT_FILE) X(STT_COMMON) X(STT_TLS)  \
                            X(STT_NUM) X(STT_LOOS) X(STT_GNU_IFUNC) X(STT_HIOS) X(STT_LOPROC) X(STT_HIPROC) \
                            X(STT_SPARC_REGISTER) X(STT_PARISC_MILLICODE) X(STT_HP_OPAQUE) X(STT_HP_STUB) X(STT_ARM_TFUNC) X(STT_ARM_16BIT)
                        #undef X
                        #define X(name) if (st_bind == name) printf(", binding %s\n", #name);
                            X(STB_LOCAL) X(STB_GLOBAL) X(STB_WEAK) X(STB_NUM) X(STB_LOOS) X(STB_GNU_UNIQUE) X(STB_HIOS) X(STB_LOPROC) \
                            X(STB_HIPROC) X(STB_MIPS_SPLIT_COMMON)
                            #undef X
#endif
                        *curr->reloc_ptr = addr;
                        break;
                    }
                }
                else
                {
                    IFDEBUG printf("parse_hash_tbl search %s hash collision with %s\n", curr->symbol_name, dt_strtab + (dt_symtab + i)->st_name);
                }
            }
            if (i == STN_UNDEF)
            {
                all_symbols_found = 0;
            }
        }
    }
    search_symbols->hashes_valid = 1;
    return all_symbols_found;
}



static ElfW(Addr) fix_dyn_ptr(ElfW(Addr) baseaddr, ElfW(Addr) ptr)
{
    if (ptr < baseaddr)
    {
        return baseaddr + ptr;
    }
    else
    {
        return ptr;
    }
}



static int parse_dynamic(ElfW(Addr) baseaddr, search_symbols_t* search_symbols, ElfW(Dyn)* dyn)
{
    const char* dt_strtab   = NULL;
    ElfW(Sym)*  dt_symtab   = NULL;
    ElfW(Word)* dt_hash     = NULL;
    ElfW(Word)* dt_gnu_hash = NULL;

    for (ElfW(Dyn)* curr = dyn; curr->d_tag != DT_NULL; curr++)
    {
        switch (curr->d_tag)
        {
            case DT_STRTAB:         // .dynstr
                dt_strtab = (const char*)fix_dyn_ptr(baseaddr, curr->d_un.d_ptr);
                break;
            case DT_SYMTAB:         // .dynsym
                dt_symtab = (ElfW(Sym)*)fix_dyn_ptr(baseaddr, curr->d_un.d_ptr);
                break;
            case DT_HASH:           // .hash
                dt_hash = (ElfW(Word)*)fix_dyn_ptr(baseaddr, curr->d_un.d_ptr);
                break;
            case DT_GNU_HASH:       // .gnu.hash
                dt_gnu_hash = (ElfW(Word)*)fix_dyn_ptr(baseaddr, curr->d_un.d_ptr);
                break;
#ifdef DEBUG
            default:
                if (0) {}
                #define X(name) else if (curr->d_tag == name) {printf("dyn.d_tag %s\n", #name);}
                    X(DT_NULL) X(DT_NEEDED) X(DT_PLTRELSZ) X(DT_PLTGOT) X(DT_HASH) X(DT_STRTAB) X(DT_SYMTAB) X(DT_RELA) \
                    X(DT_RELASZ) X(DT_RELAENT) X(DT_STRSZ) X(DT_SYMENT) X(DT_INIT) X(DT_FINI) X(DT_SONAME) \
                    X(DT_RPATH) X(DT_SYMBOLIC) X(DT_REL) X(DT_RELSZ) X(DT_RELENT) X(DT_PLTREL) X(DT_DEBUG) \
                    X(DT_TEXTREL) X(DT_JMPREL) X(DT_BIND_NOW) X(DT_RUNPATH) X(DT_LOPROC) X(DT_HIPROC) \
                    X(DT_INIT_ARRAY) X(DT_INIT_ARRAYSZ) X(DT_FINI_ARRAY) X(DT_FINI_ARRAYSZ) X(DT_FLAGS) \
                    X(DT_RELR) X(DT_RELRSZ) X(DT_RELRENT) X(DT_VERSYM) X(DT_VERNEED) X(DT_VERNEEDNUM) \
                    X(DT_RELACOUNT) X(DT_GNU_HASH) X(DT_FLAGS_1)
                #undef X
                else {
                    printf("dyn.d_tag UNKNOWN: 0x%lx\n", curr->d_tag);
                }
                break;
#endif
        }
        if (dt_symtab && dt_strtab && dt_gnu_hash)
        {
            return parse_gnu_hash_tbl(baseaddr, search_symbols, dt_gnu_hash, dt_symtab, dt_strtab);
        }
    }
    if (dt_symtab && dt_strtab && dt_hash)
    {
        return parse_hash_tbl(baseaddr, search_symbols, dt_hash, dt_symtab, dt_strtab);
    }
    return 0;
}



int elf_search_symbols_phdr(const ElfW(Addr) baseaddr, search_symbols_t* search_symbols, const ElfW(Phdr)* dlpi_phdr, ElfW(Half) dlpi_phnum)
{
    for(const ElfW(Phdr)* elf_phdr_curr = dlpi_phdr; elf_phdr_curr < dlpi_phdr + dlpi_phnum; elf_phdr_curr++)
    {
#ifdef DEBUG
        if (0) {}
        #define X(name) else if (elf_phdr_curr->p_type == name) { printf("ElfW(Phdr): %s\n", #name); }
            X(PT_NULL) X(PT_LOAD) X(PT_DYNAMIC) X(PT_INTERP) X(PT_NOTE) X(PT_SHLIB) X(PT_PHDR)
            X(PT_LOPROC) X(PT_HIPROC) X(PT_GNU_STACK) X(PT_GNU_EH_FRAME) X(PT_GNU_RELRO)
            X(PT_GNU_PROPERTY)
        #undef X
        else
        {
            printf("ElfW(Phdr): UNKNOWN TYPE 0x%x\n", elf_phdr_curr->p_type);
        }
#endif
        if (elf_phdr_curr->p_type == PT_DYNAMIC)
        {
            ElfW(Dyn)* dyn = (ElfW(Dyn)*)fix_dyn_ptr(baseaddr, elf_phdr_curr->p_vaddr);
            IFDEBUG printf("PT_DYNAMIC @ %p\n", dyn);
            return parse_dynamic(baseaddr, search_symbols, dyn);
        }
    }
    return 0;
}



int elf_search_symbols(const ElfW(Addr) baseaddr, search_symbols_t* search_symbols)
{
    ElfW(Ehdr)* elf_ehdr = (ElfW(Ehdr)*)baseaddr;
    ElfW(Phdr)* dlpi_phdr = (ElfW(Phdr)*)(baseaddr + elf_ehdr->e_phoff);
    ElfW(Half) dlpi_phnum = elf_ehdr->e_phnum;
    return elf_search_symbols_phdr(baseaddr, search_symbols, dlpi_phdr, dlpi_phnum);
}



#if 0
#define ELF_NOTE_ALIGN(size) (((size) + 3) & ~3)

static void parse_pt_note(ElfW(Addr) pt_note, size_t pt_note_sz)
{
    puts("==== parse_pt_note start ============");
    printf("pt_note_sz %zu\n", pt_note_sz);
    // -> man 5 elf
    for (ElfW(Addr) note_addr = pt_note; note_addr < pt_note + pt_note_sz;)
    {
        /* The buffer is pointing to the start of the section/segment.  */
        const ElfW(Nhdr)* note = (ElfW(Nhdr)*)note_addr;

        /* If the name is defined, it follows the note.  */
        const char* name = note->n_namesz == 0 ? NULL : (const char*)(note_addr + sizeof(*note));
        if (name)
        {
            printf("pt_note->n_name: %s\n", name);
        }
        else
        {
            printf("pt_note->n_name: none\n");
        }
#ifdef DEBUG
        if (0) {}
        #define X(name) else if (note->n_type == name) printf("Nhdr->type: %s\n", #name);
            X(NT_PRSTATUS) X(NT_FPREGSET) X(NT_PRPSINFO) X(NT_PRXREG) X(NT_TASKSTRUCT)
            X(NT_PLATFORM) X(NT_AUXV) X(NT_GWINDOWS) X(NT_ASRS) X(NT_PSTATUS)
            X(NT_PSINFO) X(NT_PRCRED) X(NT_UTSNAME) X(NT_LWPSTATUS) X(NT_LWPSINFO)
            X(NT_PRFPXREG) X(NT_SIGINFO) X(NT_FILE) X(NT_PRXFPREG) X(NT_PPC_VMX)
            X(NT_PPC_SPE) X(NT_PPC_VSX) X(NT_386_TLS) X(NT_386_IOPERM) X(NT_X86_XSTATE)
            X(NT_S390_HIGH_GPRS) X(NT_S390_TIMER) X(NT_S390_TODCMP) X(NT_S390_TODPREG)
            X(NT_S390_CTRS) X(NT_S390_PREFIX) X(NT_S390_LAST_BREAK) X(NT_S390_SYSTEM_CALL)
            X(NT_S390_TDB) X(NT_ARM_VFP) X(NT_ARM_TLS) X(NT_ARM_HW_BREAK)
            X(NT_ARM_HW_WATCH) X(NT_ARM_SYSTEM_CALL)
        #undef X
        else
        {
            printf("Nhdr->type: UNKNOWN TYPE 0x%x\n", note->n_type);
        }
#endif
        printf("note->n_namesz %u, note->n_descsz %u\n", note->n_namesz, note->n_descsz);

        /* If the descriptor is defined, it follows the name (with alignment).  */
        const char* desc = note->n_descsz == 0 ? NULL : (char*)(note_addr + sizeof(*note) + ELF_NOTE_ALIGN(note->n_namesz));

        (void)desc;

        /* The next note follows both (with alignment).  */
        note_addr += sizeof(*note) + ELF_NOTE_ALIGN(note->n_namesz) + ELF_NOTE_ALIGN(note->n_descsz);
    }
    puts("==== parse_pt_note end ==============");
}
#endif
