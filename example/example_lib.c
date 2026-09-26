#include <stdio.h>
#include <stdlib.h>
#include "dlsym-intercept.h"
#ifdef DEBUG
#define IFDEBUG
#else
#define IFDEBUG if(0)
#endif


static char intercept_lib_pfx[] = "INTERCEPT EXAMPLE LIB:";

static int my_puts(const char* s)
{
    (void)s;
    int (*real_puts)(const char*) = interceptor_get_original_addr("puts");
    if (real_puts)
    {
        return real_puts(":P");
    }
    return 0;
}



void interceptor_init()
{
    IFDEBUG printf("interceptor_init\n");

    // register hook for puts
    interceptor_register_hook("puts", my_puts);
}



static int print_dlsymd_func(const char* symbol, void* orig, void* hook, uint32_t count, void* ctx)
{
    (void)ctx;
    (void)orig;
    printf("symbol %-10s calls %u, registered %s\n",
        symbol, count, hook ? "true" : "false");
    return 0;
}



void interceptor_deinit()
{
    puts("=== example lib statistics ===\n");
    interceptor_iterate_dlsym_entries(print_dlsymd_func, NULL);
}



void interceptor_dlopen(const char* path, int flags, void* handle)
{
    printf("%s dlopen(\"%s\", %i) -> %p\n", intercept_lib_pfx, path, flags, handle);
}
