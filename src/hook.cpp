#include <map>
#include <cstddef>
#include <string>
#include "hook.h"
#include "jumptable.h"
#include "jumptable_entry.h"
#ifdef DEBUG
#include <cstdio>
#define IFDEBUG
#else
#define IFDEBUG  if (0)
#endif



typedef struct {
    void* orig;             // original function address
    void* hook;             // user supplied hook
    void* jumptable_entry;  // address of jumptable_entry
} dlsym_entry_t;



typedef std::map<std::string, dlsym_entry_t> hook_map_t;
/* reserve space for map. we init the map in hook_init_internal() to enforce
   the correct init sequence. if the map is defined global, its constructor
   may be called AFTER the lib init ...*/
char hook_map_buffer[sizeof(hook_map_t)];



hook_map_t& get_hook_map()
{
    return *((hook_map_t*)&hook_map_buffer);
}



extern "C" {

void hook_init_internal(void)
{
    new (&hook_map_buffer) hook_map_t();
}



void hook_deinit_internal(void)
{
    (&get_hook_map())->hook_map_t::~hook_map_t();
}




/*
called by user to get original addr of function
*/
void* interceptor_get_original_addr(const char* symbol)
{
    auto it = get_hook_map().find(symbol);
    if (it != get_hook_map().end())
    {
        return it->second.orig;
    }
    else
    {
        IFDEBUG printf("interceptor_get_original_addr(%s) MISSING ORIGNAL ADDR! return NULL\n", symbol);
    }
    return NULL;
}



/*
called by user code to register hook
*/
int interceptor_register_hook(const char* symbol, void* hook)
{
    auto it = get_hook_map().find(symbol);
    if (it == get_hook_map().end()) // not registerd atm
    {
        void* jumptable_entry = jumptable_add_entry(hook);
        get_hook_map().insert({symbol, {NULL, hook, jumptable_entry}});
        return 1;
    }
    else
    {
        IFDEBUG printf("function %s was registerd before or already called\n", symbol);
        return 0;
    }
}



/*
    called by fake_dlsym
*/
void* hook_add_dlsym_entry(const char* symbol, void* orig)
{
    auto it = get_hook_map().find(symbol);
    if (it != get_hook_map().end()) // hook registerd
    {
        it->second.orig = orig;
        void* code_addr = jumptable_entry_get_code_addr(it->second.jumptable_entry);
        return code_addr;
    }
    else  // no hook registerd
    {
        void* jumptable_entry = jumptable_add_entry(orig);
        void* code_addr = jumptable_entry_get_code_addr(jumptable_entry);
        get_hook_map().insert({symbol, {orig, NULL, jumptable_entry}});
        return code_addr;
    }
}



void interceptor_iterate_dlsym_entries(int(*cb)(const char* symbol, void* orig, void* hook, uint32_t cnt, void* ctx), void* ctx)
{
    for (auto it = get_hook_map().begin(); it != get_hook_map().end(); it++)
    {
        uint32_t cnt = jumptable_entry_get_count(it->second.jumptable_entry);
        if (cb(it->first.c_str(), it->second.orig, it->second.hook, cnt, ctx))
        {
            break;
        }
    }
}


} // extern "C"
