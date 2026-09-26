#define _GNU_SOURCE
#define _POSIX_C_SOURCE  200809L
#include <stdio.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include "jumptable.h"
#include "jumptable_entry.h"
#include "dlsym-intercept.h"
#include "hook.h"


static char*    jumptable = NULL;
static uint32_t jumptable_entries_max = 0;
static uint32_t jumptable_entries = 0;




static size_t jumptable_entry_template_size()
{
    return jumptable_entry_template_end - jumptable_entry_template_begin;
}


static void* get_jumptable_entry(uint32_t num)
{
    return jumptable + num * jumptable_entry_template_size();
}


int interceptor_set_element_num(uint32_t entries_num)
{
    if(jumptable)
    {
        dprintf(STDERR_FILENO, "jumptable_set_size can only be used before jumptable_create\n");
        return 0;
    }
    else
    {
        jumptable_entries_max = entries_num;
        return 1;
    }
}



static size_t calc_mapping_size(uint32_t entries_num)
{
    size_t mmap_size = jumptable_entry_template_size() * entries_num;
    size_t page_size = sysconf(_SC_PAGE_SIZE);
    mmap_size = mmap_size + page_size - 1;
    mmap_size &= (~(page_size - 1));
    return mmap_size;
}



int jumptable_create()
{
    if (!jumptable)
    {
        size_t mmap_size = calc_mapping_size(jumptable_entries_max);
        jumptable = mmap(NULL, mmap_size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (jumptable)
        {
            jumptable_entries_max = mmap_size / jumptable_entry_template_size();
            return 1;
        }
        else
        {
            dprintf(STDERR_FILENO, "jumptable_create mmap: %s\n", strerror(errno));
        }
    }
    else
    {
        dprintf(STDERR_FILENO, "jumptable_create: already initialized\n");
    }
    return 0;
}



int jumptable_delete()
{
    if (jumptable)
    {
        size_t mmap_size = calc_mapping_size(jumptable_entries_max);
        munmap(jumptable, mmap_size);
        jumptable = NULL;
        jumptable_entries_max = 0;
    }
    return 1;
}



void* jumptable_add_entry(void* func)
{
    if (jumptable)
    {
        if (jumptable_entries < jumptable_entries_max)
        {
            void* jumptable_entry = get_jumptable_entry(jumptable_entries);
            memcpy(jumptable_entry, jumptable_entry_template_begin, jumptable_entry_template_size());
            jumptable_entry_init(jumptable_entry, func);
            jumptable_entries++;
            return jumptable_entry;
        }
        else
        {
            dprintf(STDERR_FILENO, "jumptable_add all slots full. init bigger jumptable.\n");
        }
    }
    else
    {
        dprintf(STDERR_FILENO, "jumptable_uninitialized\n");
    }
    return NULL;
}
