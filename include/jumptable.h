#pragma once
#include <stdint.h>


#ifdef __cplusplus
extern "C" {
#endif



int         jumptable_create();
int         jumptable_delete();
void*       jumptable_add_entry(void* func);

#ifdef __cplusplus
} // extern "C"
#endif
