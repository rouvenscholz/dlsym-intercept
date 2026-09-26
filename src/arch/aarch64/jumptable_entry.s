

.macro gen_symbol symbol_name
.global \symbol_name
.hidden \symbol_name
\symbol_name:
.endm



.section .rodata
gen_symbol jumptable_entry_template_begin
jumptable_entry_call_destination:
.quad 0
jumptable_entry_call_count:
.long 0
jumptable_entry_call_code:
    adr     x9, jumptable_entry_call_destination
.equ offset, jumptable_entry_template_begin-jumptable_entry_call_count
    ldp     x10, x11, [x9]
    add     w11, w11, 1
    str     w11, [x9, jumptable_entry_call_count-jumptable_entry_call_destination]
    br      x10
gen_symbol jumptable_entry_template_end

.text

# void jumptable_entry_init(void* jumptable_entry, void* destination);
gen_symbol jumptable_entry_init
.equ offset, jumptable_entry_call_destination-jumptable_entry_template_begin
    str     x1, [x0, offset]
    ret



#void* jumptable_entry_get_code_addr(void* jumptable_entry);
gen_symbol jumptable_entry_get_code_addr
.equ offset, jumptable_entry_call_code-jumptable_entry_template_begin
     add    x0, x0, offset
     ret



# uint32_t jumptable_entry_get_count(void* jumptable_entry)
gen_symbol jumptable_entry_get_count
.equ offset, jumptable_entry_call_count-jumptable_entry_template_begin
    ldr     w0, [x0, offset]
    ret
