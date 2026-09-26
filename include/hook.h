#pragma once
#ifdef __cplusplus
extern "C" {
#endif


void  hook_init_internal(void);
void  hook_deinit_internal(void);
void* hook_add_dlsym_entry(const char* symbol, void* orig);


#ifdef __cplusplus
}  // extern "C"
#endif
