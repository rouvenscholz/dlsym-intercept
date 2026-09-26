#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// template start
extern const char jumptable_entry_template_begin[];
// template end
extern const char jumptable_entry_template_end[];

// set destination of jumptable_entry. return entry of code
void jumptable_entry_init(void* jumptable_entry, void* destination);

// get code addr of jumptable_entry
void* jumptable_entry_get_code_addr(void* jumptable_entry);

// return number of calles
uint32_t jumptable_entry_get_count(void* jumptable_entry);

#ifdef __cplusplus
} // extern "C"
#endif
