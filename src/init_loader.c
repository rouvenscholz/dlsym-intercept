#define _GNU_SOURCE
#include <stdio.h>
#include <link.h>
#include <elf.h>
#include <libelf.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "hook.h"
#include "jumptable.h"
#include "dlsym-intercept.h"
#include "get_elf_header.h"
#include "search_symbols.h"




#ifdef DEBUG
#define IFDEBUG
#else
#define IFDEBUG if(0)
#endif



void*    (*real_dlopen)  (const char *path, int flags) = NULL;
int      (*real_dlclose) (void* handle) = NULL;
void*    (*real_dlsym)   (void *restrict handle, const char *restrict symbol) = NULL;
void*    (*real_dlvsym)  (void *restrict handle, const char *restrict symbol, const char *restrict version) = NULL;



extern void interceptor_init(void) __attribute__ ((weak, alias ("default_interceptor_init")));
extern void interceptor_deinit(void) __attribute__ ((weak, alias ("default_interceptor_deinit")));
extern void interceptor_dlopen(const char* path, int flags, void* handle) __attribute__ ((weak, alias ("default_interceptor_dlopen")));
static void load_and_parse();



__attribute__((used))
static void default_interceptor_init() {
    IFDEBUG printf("no user defined interceptor_init\n");
}



__attribute__((used))
static void default_interceptor_deinit() {
    IFDEBUG printf("no user defined interceptor_deinit\n");
}



__attribute__((constructor))
void lib_init(void)
{
    IFDEBUG printf("lib_init\n");
    load_and_parse();
    interceptor_set_element_num(1024);
    hook_init_internal();
    jumptable_create();
    interceptor_init();
}



__attribute__((destructor))
void lib_deinit(void)
{
    interceptor_deinit();
    IFDEBUG printf("lib_deinit\n");
    hook_deinit_internal();
    jumptable_delete();
}



// Fake dlsym
__attribute__ ((visibility ("default")))
void* dlsym(void *restrict handle, const char *restrict symbol)
{
    void* addr = real_dlsym(handle, symbol);
    IFDEBUG printf("dlsym %s -> %p\n", symbol, addr);
    void* func = NULL;
    if (addr)
    {
        func = hook_add_dlsym_entry(symbol, addr);
    }
    return func;
}



__attribute__((used))
static void default_interceptor_dlopen(const char* path, int flags, void* handle)
{
    (void)path;
    (void)flags;
    (void)handle;
    IFDEBUG printf("no user defined interceptor_dlopen\n");
}



// Fake dlopen
__attribute__ ((visibility ("default")))
void* dlopen(const char* path, int flags)
{
    IFDEBUG printf("dlopen(\"%s\", %i)\n", path, flags);
    void* handle = real_dlopen(path, flags);
    interceptor_dlopen(path, flags, handle);
    return handle;
}



static int dl_iterate_phdr_cb(struct dl_phdr_info* info, size_t size, void* ctx)
{
    (void)size;
    search_symbols_t* search_symbols = (search_symbols_t*)ctx;
    if (info->dlpi_addr == (ElfW(Addr))get_elf_header())
    {
        IFDEBUG printf("ignore own lib: \"%s\"\n", info->dlpi_name);
        return 0;
    }
    else
    {
        IFDEBUG printf("search next elf: \"%s\"\n", info->dlpi_name);
        return elf_search_symbols_phdr(info->dlpi_addr, search_symbols, info->dlpi_phdr, info->dlpi_phnum);
    }
}



static void load_and_parse()
{
    IFDEBUG printf("lib_init\n");

    search_symbol_entry_t search_symbol_entries[] = {
        {
            .symbol_name = "dlopen",
            .reloc_ptr = (void**)&real_dlopen,
        },
        {
            .symbol_name = "dlclose",
            .reloc_ptr = (void**)&real_dlclose,
        },
        {
            .symbol_name = "dlsym",
            .reloc_ptr = (void**)&real_dlsym,
        },
        {
            .symbol_name = "dlvsym",
            .reloc_ptr = (void**)&real_dlvsym,
        },
    };
    search_symbols_t search_symbols = {
        .search_symbols_entries = (search_symbol_entry_t*)&search_symbol_entries,
        .search_symbols_entries_num = sizeof(search_symbol_entries) / sizeof(search_symbol_entry_t),
        .hashes_valid = 0,
        .gnu_hashes_valid = 0,
    };


    dl_iterate_phdr(dl_iterate_phdr_cb, &search_symbols);


# if 0
    void** plt_got = get_plt_got();


    struct link_map* self = (struct link_map*)plt_got[1];
    struct link_map* curr = self;
    //for(; curr->l_prev; curr = curr->l_prev);
    for(; curr; curr = curr->l_next)
    {
        IFDEBUG printf("elf obj \"%s\", baseaddr: 0x%lx\n", curr->l_name, curr->l_addr);
        if (curr == self)
        {
            IFDEBUG printf("own elf obj -> continue with next\n");
        }
        else
        {
            if (parse_dynamic(&search_funcs, curr->l_addr, curr->l_ld))
            {
                IFDEBUG printf("all symbols found\n");
                return;
            }
        }
    }

    dprintf(STDERR_FILENO, "Could not locate symbols:");
    int is_first = 1;
    for (search_funcs_entry_t* entry = (search_funcs_entry_t*)&search_funcs_entrys; entry < ((search_funcs_entry_t*)&search_funcs_entrys) + (sizeof(search_funcs_entrys) / sizeof(search_funcs_entry_t)); entry++)
    {
        if (!*entry->reloc_ptr)
        {
            dprintf(STDERR_FILENO, "%s \"%s\"", is_first ? "" : ",", entry->symbol_name);
            is_first = 0;
        }
    }
    dprintf(STDERR_FILENO, "\n");
    exit(EXIT_FAILURE);
#endif
}
