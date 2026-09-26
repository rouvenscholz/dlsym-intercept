#include <stdio.h>
#include <dlfcn.h>
#include <stdlib.h>



int main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    const char lib[] = "libc.so.6";
    void* handle = dlopen(lib, RTLD_LAZY);
    if (handle)
    {
        int(*pf)(const char*, ...) =  dlsym(handle, "printf");
        pf("Hello world 1\n");
        int(*_puts)(const char*) = dlsym(handle, "puts");
        if (_puts)
        {
            _puts("Hello world 2");
            return EXIT_SUCCESS;
        }
        else
        {
            fprintf(stderr, "%s\n", dlerror());
        }
        dlclose(handle);
    }
    else
    {
        fprintf(stderr, "%s\n", dlerror());
    }
    return EXIT_FAILURE;
}
