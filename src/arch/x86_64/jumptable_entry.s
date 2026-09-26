

.macro gen_symbol symbol_name
.global \symbol_name
.hidden \symbol_name
\symbol_name:
.endm



.section .rodata
.align 8
gen_symbol jumptable_entry_template_begin
jumptable_entry_call_destination:
.quad 0
jumptable_entry_call_count:
.long 0
jumptable_entry_call_code:
    lea     jumptable_entry_call_destination(%rip), %rax
    incl    jumptable_entry_call_count - jumptable_entry_call_destination(%rax)
    jmp     *(%rax)
.align 8
gen_symbol jumptable_entry_template_end



.text

# void jumptable_entry_init(void* jumptable_entry, void* destination);
gen_symbol jumptable_entry_init
    mov     %rsi, jumptable_entry_call_destination-jumptable_entry_template_begin(%rdi)
    ret


# void* jumptable_entry_get_code_addr(void* jumptable_entry);
gen_symbol jumptable_entry_get_code_addr
    lea     jumptable_entry_call_code - jumptable_entry_template_begin(%rdi), %rax
    ret



# uint32_t jumptable_entry_get_count(void* jumptable_entry)
gen_symbol jumptable_entry_get_count
    mov     jumptable_entry_call_count - jumptable_entry_template_begin(%rdi), %eax
    ret
