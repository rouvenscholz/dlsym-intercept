#pragma once
#include <stdint.h>


// real dlclose (if needed)
extern int (*real_dlclose) (void* handle);

// real dl_open (if needed)
extern void*(*real_dlopen)(const char *path, int flags);

// real dl_sym (if needed)
extern void*(*real_dlsym)(void* restrict handle, const char* restrict symbol);

// real dlvsym (if needed)
extern void* (*real_dlvsym)  (void *restrict handle, const char *restrict symbol, const char *restrict version);

// function that could be overloaded by user, for libinit
void    interceptor_init();

// function that could be overloaded by user, for libinit
void    interceptor_deinit();

// function that could be overloaded by user, called after every dlopen
void    interceptor_dlopen(const char* path, int flags, void* handle);

// change number of possible entries. default 1024
int     interceptor_set_element_num(uint32_t element_num);

// add hook for symbol
int     interceptor_register_hook(const char* symbol, void* hook);

// get original symbol addr
void*   interceptor_get_original_addr(const char* symbol);

// iterate over all dlsymd functions and return call count
void    interceptor_iterate_dlsym_entries(int(*cb)(const char* symbol, void* orig, void* hook, uint32_t cnt, void* ctx), void* ctx);
