#pragma once


typedef struct {
    const char* symbol_name;
    ElfW(Word)  symbol_hash;
    ElfW(Word)  symbol_gnu_hash;
    void**      reloc_ptr;
} search_symbol_entry_t;



typedef struct {
    search_symbol_entry_t*      search_symbols_entries;
    unsigned int                search_symbols_entries_num;
    unsigned int                hashes_valid:1;
    unsigned int                gnu_hashes_valid:1;
} search_symbols_t;




int elf_search_symbols(const ElfW(Addr) baseaddr, search_symbols_t* search_symbols);
int elf_search_symbols_phdr(const ElfW(Addr) baseaddr, search_symbols_t* search_symbols, const ElfW(Phdr)* dlpi_phdr, ElfW(Half) dlpi_phnum);
